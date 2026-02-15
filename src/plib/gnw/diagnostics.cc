#include "plib/gnw/diagnostics.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>

#include <SDL.h>

#include "game/cache.h"
#include "plib/color/color.h"
#include "plib/db/db.h"
#include "plib/gnw/memory.h"
#include "plib/gnw/svga.h"
#include "plib/gnw/text.h"

namespace fallout {

namespace {

    constexpr int kHudX = 6;
    constexpr int kHudY = 6;
    constexpr int kHudPadding = 4;
    constexpr int kHudLineSpacing = 1;
    constexpr int kHudMaxWidth = 460;
    constexpr int kHudMaxHeight = 180;
    constexpr int kHudFixedWidth = 452;
    constexpr int kHudHeaderVerticalPadding = 1;
    constexpr int kHudSectionGap = 2;
    constexpr int kHudSparklineHeight = 16;
    constexpr float kHudPerfSnapshotHz = 4.0f;
    constexpr int kLoadPhaseCapacity = 16;
    constexpr int kFrameSampleCapacity = 1024;
    constexpr float kFrameWindowSeconds = 10.0f;
    // Ignore large transition/load stalls so pacing reflects active runtime.
    constexpr float kPacingOutlierCutoffMs = 90.0f;
    constexpr bool kForceHudAlwaysOn = false;

    struct LoadPhase {
        char name[64];
        float elapsedMs;
    };

    struct FrameSample {
        float frameTimeMs;
        Uint64 counter;
    };

    static bool gDiagnosticsInitialized = false;
    static int gDiagnosticsMode = DIAGNOSTICS_MODE_OFF;
    static Uint64 gPerformanceFrequency = 0;
    static Uint64 gLastPresentCounter = 0;
    static Uint64 gLastLogCounter = 0;
    static float gFrameTimeMs = 0.0f;
    static float gSmoothedFrameTimeMs = 0.0f;
    static float gSmoothedFps = 0.0f;
    static float gDisplayedMemMiB = 0.0f;
    static float gDisplayedPeakMemMiB = 0.0f;
    static Uint64 gHudPerfSnapshotCounter = 0;
    static float gHudPerfFps = 0.0f;
    static float gHudPerfFrameMs = 0.0f;
    static float gHudPerfMemMiB = 0.0f;
    static float gHudPerfPeakMemMiB = 0.0f;
    static FrameSample gFrameSamples[kFrameSampleCapacity];
    static int gFrameSampleHead = 0;
    static int gFrameSampleCount = 0;
    static float gFrameP50Ms = 0.0f;
    static float gFrameP95Ms = 0.0f;
    static float gFrameP99Ms = 0.0f;
    static float gWorstFrame10sMs = 0.0f;
    static unsigned long long gHitchOver33Count = 0;
    static unsigned long long gHitchOver50Count = 0;

    static unsigned long long gCacheLockHits = 0;
    static unsigned long long gCacheLockMisses = 0;
    static unsigned long long gCacheEvictions = 0;
    static unsigned long long gCacheFlushes = 0;
    static unsigned long long gCacheLoadSamples = 0;
    static float gCacheAverageLoadMs = 0.0f;
    static float gCacheMaxLoadMs = 0.0f;
    static float gCacheHitRate = 0.0f;

    static Uint64 gIoLastSampleCounter = 0;
    static unsigned long long gIoLastSampleBytes = 0;
    static float gIoReadKiBPerSec = 0.0f;
    static unsigned long long gIoReadBytesTotal = 0;
    static unsigned long long gIoReadOpsTotal = 0;
    static unsigned long long gIoSlowReadOps = 0;
    static float gIoSlowestReadMs = 0.0f;

    static Uint64 gLoadStartCounter = 0;
    static char gLoadProfileName[32];
    static LoadPhase gLoadPhases[kLoadPhaseCapacity];
    static int gLoadPhaseCount = 0;

    static bool gHudWasDrawn = false;
    static SDL_Rect gLastHudRect = {};
    static unsigned char gHudBackup[kHudMaxWidth * kHudMaxHeight];

    static float countersToMs(Uint64 ticks)
    {
        if (gPerformanceFrequency == 0) {
            return 0.0f;
        }

        return static_cast<float>(ticks * 1000.0 / static_cast<double>(gPerformanceFrequency));
    }

    static void sanitizePhaseName(const char* input, char* output, size_t outputSize)
    {
        if (outputSize == 0) {
            return;
        }

        if (input == NULL) {
            output[0] = '\0';
            return;
        }

        while (*input != '\0' && (isspace(static_cast<unsigned char>(*input)) || *input == '>')) {
            input++;
        }

        size_t index = 0;
        while (*input != '\0' && index + 1 < outputSize) {
            char ch = *input++;
            if (ch == '\r' || ch == '\n' || ch == '\t') {
                break;
            }

            output[index++] = ch;
        }

        while (index > 0 && output[index - 1] == ' ') {
            index--;
        }

        output[index] = '\0';
    }

    static void logModeState()
    {
        printf("[diag] mode hud=%s\n",
            (gDiagnosticsMode & DIAGNOSTICS_MODE_HUD) != 0 ? "on" : "off");
        fflush(stdout);
    }

    static int applyModeConstraints(int mode)
    {
        int constrainedMode = mode & DIAGNOSTICS_MODE_HUD;
        if (kForceHudAlwaysOn) {
            constrainedMode |= DIAGNOSTICS_MODE_HUD;
        }

        return constrainedMode;
    }

    static float smoothValue(float previous, float current)
    {
        if (previous <= 0.0f) {
            return current;
        }

        return previous * 0.8f + current * 0.2f;
    }

    static float smoothValueWithAlpha(float previous, float current, float alpha)
    {
        if (previous <= 0.0f) {
            return current;
        }

        return previous * alpha + current * (1.0f - alpha);
    }

    static int clampColorComponent(int value)
    {
        if (value < 0) {
            return 0;
        }

        if (value > 31) {
            return 31;
        }

        return value;
    }

    static unsigned char paletteColorFromRgb5(int r, int g, int b)
    {
        const int colorIndex = (clampColorComponent(r) << 10)
            | (clampColorComponent(g) << 5)
            | clampColorComponent(b);
        return static_cast<unsigned char>(colorTable[colorIndex]);
    }

    static void addFrameSample(float frameTimeMs, Uint64 now)
    {
        gFrameSamples[gFrameSampleHead].frameTimeMs = frameTimeMs;
        gFrameSamples[gFrameSampleHead].counter = now;
        gFrameSampleHead = (gFrameSampleHead + 1) % kFrameSampleCapacity;
        if (gFrameSampleCount < kFrameSampleCapacity) {
            gFrameSampleCount++;
        }
    }

    static float percentileFromSortedValues(const float* values, int count, float percentile)
    {
        if (values == NULL || count <= 0) {
            return 0.0f;
        }

        const float normalizedPercentile = std::max(0.0f, std::min(100.0f, percentile)) / 100.0f;
        const float position = normalizedPercentile * static_cast<float>(count - 1);
        const int lowerIndex = static_cast<int>(position);
        const int upperIndex = std::min(lowerIndex + 1, count - 1);
        const float fraction = position - lowerIndex;
        return values[lowerIndex] + (values[upperIndex] - values[lowerIndex]) * fraction;
    }

    static int collectRecentFrameSamples(Uint64 now, float* frameSamples, int capacity)
    {
        if (frameSamples == NULL || capacity <= 0 || gFrameSampleCount == 0 || gPerformanceFrequency == 0) {
            return 0;
        }

        const Uint64 windowCounters = static_cast<Uint64>(gPerformanceFrequency * kFrameWindowSeconds);
        int sampleWriteIndex = 0;

        int index = gFrameSampleHead - gFrameSampleCount;
        if (index < 0) {
            index += kFrameSampleCapacity;
        }

        for (int sampleIndex = 0; sampleIndex < gFrameSampleCount; sampleIndex++) {
            const FrameSample& sample = gFrameSamples[index];
            if (sample.counter <= now && now - sample.counter <= windowCounters && sampleWriteIndex < capacity) {
                frameSamples[sampleWriteIndex++] = sample.frameTimeMs;
            }

            index++;
            if (index >= kFrameSampleCapacity) {
                index = 0;
            }
        }

        return sampleWriteIndex;
    }

    static void refreshFramePacingMetrics(Uint64 now)
    {
        gFrameP50Ms = 0.0f;
        gFrameP95Ms = 0.0f;
        gFrameP99Ms = 0.0f;
        gWorstFrame10sMs = 0.0f;

        float recentFrames[kFrameSampleCapacity];
        int recentFrameCount = collectRecentFrameSamples(now, recentFrames, kFrameSampleCapacity);

        if (recentFrameCount == 0) {
            return;
        }

        for (int sampleIndex = 0; sampleIndex < recentFrameCount; sampleIndex++) {
            if (recentFrames[sampleIndex] > gWorstFrame10sMs) {
                gWorstFrame10sMs = recentFrames[sampleIndex];
            }
        }

        std::sort(recentFrames, recentFrames + recentFrameCount);
        gFrameP50Ms = percentileFromSortedValues(recentFrames, recentFrameCount, 50.0f);
        gFrameP95Ms = percentileFromSortedValues(recentFrames, recentFrameCount, 95.0f);
        gFrameP99Ms = percentileFromSortedValues(recentFrames, recentFrameCount, 99.0f);
    }

    static void updateCacheMetrics()
    {
        cache_get_diagnostics(&gCacheLockHits,
            &gCacheLockMisses,
            &gCacheEvictions,
            &gCacheFlushes,
            &gCacheLoadSamples,
            &gCacheAverageLoadMs,
            &gCacheMaxLoadMs);
        const unsigned long long totalLocks = gCacheLockHits + gCacheLockMisses;
        if (totalLocks == 0) {
            gCacheHitRate = 0.0f;
            return;
        }

        gCacheHitRate = static_cast<float>(100.0 * static_cast<double>(gCacheLockHits) / static_cast<double>(totalLocks));
    }

    static void updateIoMetrics(Uint64 now)
    {
        unsigned long long readBytes = 0;
        db_get_io_diagnostics(&readBytes, &gIoReadOpsTotal, &gIoSlowReadOps, &gIoSlowestReadMs);
        gIoReadBytesTotal = readBytes;

        if (gIoLastSampleCounter != 0 && now > gIoLastSampleCounter) {
            const float elapsedSeconds = countersToMs(now - gIoLastSampleCounter) / 1000.0f;
            if (elapsedSeconds > 0.0f && readBytes >= gIoLastSampleBytes) {
                const float readKiB = static_cast<float>(readBytes - gIoLastSampleBytes) / 1024.0f;
                const float instantReadRate = readKiB / elapsedSeconds;
                gIoReadKiBPerSec = smoothValue(gIoReadKiBPerSec, instantReadRate);
            }
        }

        gIoLastSampleCounter = now;
        gIoLastSampleBytes = readBytes;
    }

    static void updateFrameMetrics(Uint64 now)
    {
        if (gLastPresentCounter != 0) {
            gFrameTimeMs = countersToMs(now - gLastPresentCounter);
            gSmoothedFrameTimeMs = smoothValue(gSmoothedFrameTimeMs, gFrameTimeMs);

            if (gSmoothedFrameTimeMs > 0.0f) {
                gSmoothedFps = 1000.0f / gSmoothedFrameTimeMs;
            }

            if (gFrameTimeMs < kPacingOutlierCutoffMs) {
                addFrameSample(gFrameTimeMs, now);

                if (gFrameTimeMs > 33.0f) {
                    gHitchOver33Count++;
                }

                if (gFrameTimeMs > 50.0f) {
                    gHitchOver50Count++;
                }
            }

            refreshFramePacingMetrics(now);
        }

        gLastPresentCounter = now;
    }

    static void emitPeriodicLog(Uint64 now)
    {
        gLastLogCounter = now;
    }

    static void copyRectToBackup(int x, int y, int width, int height)
    {
        unsigned char* src = static_cast<unsigned char*>(gSdlSurface->pixels) + y * gSdlSurface->pitch + x;

        for (int row = 0; row < height; row++) {
            memcpy(gHudBackup + row * width, src + row * gSdlSurface->pitch, width);
        }
    }

    static void restoreRectFromBackup(int x, int y, int width, int height)
    {
        unsigned char* dst = static_cast<unsigned char*>(gSdlSurface->pixels) + y * gSdlSurface->pitch + x;

        for (int row = 0; row < height; row++) {
            memcpy(dst + row * gSdlSurface->pitch, gHudBackup + row * width, width);
        }
    }

    static void fillRectIndexed(int x, int y, int width, int height, unsigned char color)
    {
        unsigned char* dst = static_cast<unsigned char*>(gSdlSurface->pixels) + y * gSdlSurface->pitch + x;
        for (int row = 0; row < height; row++) {
            memset(dst + row * gSdlSurface->pitch, color, width);
        }
    }

    static void setPixelIndexed(int x, int y, unsigned char color)
    {
        unsigned char* dst = static_cast<unsigned char*>(gSdlSurface->pixels) + y * gSdlSurface->pitch + x;
        *dst = color;
    }

    static int sparklineYForFrameMs(float frameMs, int sparklineY, int sparklineHeight)
    {
        constexpr float kSparklineMinMs = 8.0f;
        constexpr float kSparklineMaxMs = 50.0f;
        const float clampedMs = std::max(kSparklineMinMs, std::min(kSparklineMaxMs, frameMs));
        const float normalized = (clampedMs - kSparklineMinMs) / (kSparklineMaxMs - kSparklineMinMs);
        const int y = sparklineY + sparklineHeight - 1 - static_cast<int>(normalized * static_cast<float>(sparklineHeight - 1));
        return y;
    }

    static void drawFrameSparkline(int x,
        int y,
        int width,
        int height,
        unsigned char backgroundColor,
        unsigned char gridColor,
        unsigned char goodColor,
        unsigned char warnColor,
        unsigned char badColor)
    {
        if (width <= 2 || height <= 2) {
            return;
        }

        fillRectIndexed(x, y, width, height, backgroundColor);

        const int targetFrameY = sparklineYForFrameMs(16.67f, y, height);
        const int hitchFrameY = sparklineYForFrameMs(33.0f, y, height);
        fillRectIndexed(x, targetFrameY, width, 1, gridColor);
        fillRectIndexed(x, hitchFrameY, width, 1, gridColor);

        float recentFrames[kFrameSampleCapacity];
        const int recentFrameCount = collectRecentFrameSamples(SDL_GetPerformanceCounter(), recentFrames, kFrameSampleCapacity);
        if (recentFrameCount <= 0) {
            return;
        }

        int previousY = 0;
        bool hasPreviousPoint = false;

        for (int pixel = 0; pixel < width; pixel++) {
            int sampleIndex = static_cast<int>((static_cast<long long>(pixel) * recentFrameCount) / width);
            if (sampleIndex >= recentFrameCount) {
                sampleIndex = recentFrameCount - 1;
            }

            const float frameMs = recentFrames[sampleIndex];
            const int pointY = sparklineYForFrameMs(frameMs, y, height);

            unsigned char pointColor = goodColor;
            if (frameMs > 50.0f) {
                pointColor = badColor;
            } else if (frameMs > 33.0f) {
                pointColor = warnColor;
            }

            if (hasPreviousPoint) {
                const int lineTop = std::min(previousY, pointY);
                const int lineBottom = std::max(previousY, pointY);
                fillRectIndexed(x + pixel, lineTop, 1, lineBottom - lineTop + 1, pointColor);
            } else {
                setPixelIndexed(x + pixel, pointY, pointColor);
            }

            previousY = pointY;
            hasPreviousPoint = true;
        }
    }

    static void clearHudArtifactIfNeeded()
    {
        if (!gHudWasDrawn) {
            return;
        }

        SDL_BlitSurface(gSdlSurface, &gLastHudRect, gSdlTextureSurface, &gLastHudRect);
        gHudWasDrawn = false;
    }

    static void renderHudOverlay()
    {
        if ((gDiagnosticsMode & DIAGNOSTICS_MODE_HUD) == 0) {
            clearHudArtifactIfNeeded();
            return;
        }

        if (gSdlSurface == NULL || gSdlTextureSurface == NULL || text_to_buf == NULL || text_width == NULL || text_height == NULL) {
            return;
        }

        char headerLine[96];
        char perfLine[128];
        char pacingLine[128];
        char hitchLine[96];
        char cacheLine[140];
        char ioLine[140];
        char loadLine[120];
        int lineColors[6];

        const float rawMemMiB = static_cast<float>(static_cast<double>(mem_get_allocated()) / (1024.0 * 1024.0));
        const float rawPeakMemMiB = static_cast<float>(static_cast<double>(mem_get_peak_allocated()) / (1024.0 * 1024.0));
        gDisplayedMemMiB = smoothValueWithAlpha(gDisplayedMemMiB, rawMemMiB, 0.92f);
        gDisplayedPeakMemMiB = smoothValueWithAlpha(gDisplayedPeakMemMiB, rawPeakMemMiB, 0.92f);

        const Uint64 hudNow = SDL_GetPerformanceCounter();
        const Uint64 perfSnapshotInterval = gPerformanceFrequency > 0
            ? static_cast<Uint64>(static_cast<double>(gPerformanceFrequency) / kHudPerfSnapshotHz)
            : 0;
        if (gHudPerfSnapshotCounter == 0 || perfSnapshotInterval == 0 || hudNow - gHudPerfSnapshotCounter >= perfSnapshotInterval) {
            gHudPerfFps = gSmoothedFps;
            gHudPerfFrameMs = gSmoothedFrameTimeMs;
            gHudPerfMemMiB = gDisplayedMemMiB;
            gHudPerfPeakMemMiB = gDisplayedPeakMemMiB;
            gHudPerfSnapshotCounter = hudNow;
        }

        const float perfFpsDisplay = static_cast<float>(round(gHudPerfFps * 10.0f) / 10.0f);
        const float perfFrameMsDisplay = static_cast<float>(round(gHudPerfFrameMs * 100.0f) / 100.0f);
        const float perfMemMiBDisplay = static_cast<float>(round(gHudPerfMemMiB * 10.0f) / 10.0f);
        const float perfPeakMemMiBDisplay = static_cast<float>(round(gHudPerfPeakMemMiB * 10.0f) / 10.0f);

        headerLine[0] = '\0';
        snprintf(perfLine, sizeof(perfLine), "FPS %5.1f (%5.2f ms)  Mem %6.1f/%6.1f MiB", perfFpsDisplay, perfFrameMsDisplay, perfMemMiBDisplay, perfPeakMemMiBDisplay);
        snprintf(pacingLine, sizeof(pacingLine), "Pace p50/p95/p99 %.2f/%.2f/%.2f ms  worst10 %.2f", gFrameP50Ms, gFrameP95Ms, gFrameP99Ms, gWorstFrame10sMs);
        snprintf(hitchLine, sizeof(hitchLine), "Hitches >33ms %llu  >50ms %llu", gHitchOver33Count, gHitchOver50Count);

        snprintf(cacheLine,
            sizeof(cacheLine),
            "Cache hit %.1f%%  load %.2f/%.2fms",
            gCacheHitRate,
            gCacheAverageLoadMs,
            gCacheMaxLoadMs);
        snprintf(ioLine, sizeof(ioLine), "I/O %.0f KiB/s  max %.1fms", gIoReadKiBPerSec, gIoSlowestReadMs);
        if (gLoadPhaseCount > 0) {
            const LoadPhase& phase = gLoadPhases[gLoadPhaseCount - 1];
            snprintf(loadLine, sizeof(loadLine), "Load %s @ %.1f ms", phase.name, phase.elapsedMs);
        } else {
            snprintf(loadLine, sizeof(loadLine), "Load phase data unavailable");
        }

        const unsigned char panelBackgroundColor = paletteColorFromRgb5(2, 2, 4);
        const unsigned char panelInnerBackgroundColor = paletteColorFromRgb5(1, 1, 2);
        const unsigned char headerBackgroundColor = paletteColorFromRgb5(5, 9, 16);
        const unsigned char headerAccentColor = paletteColorFromRgb5(12, 19, 30);
        const unsigned char outerBorderColor = paletteColorFromRgb5(22, 26, 31);
        const unsigned char innerBorderColor = paletteColorFromRgb5(7, 10, 16);
        const unsigned char sparkGridColor = paletteColorFromRgb5(6, 8, 12);
        const unsigned char sparkGoodColor = paletteColorFromRgb5(12, 27, 14);
        const unsigned char sparkWarnColor = paletteColorFromRgb5(30, 24, 8);
        const unsigned char sparkBadColor = paletteColorFromRgb5(31, 10, 10);

        const int goodTextColor = static_cast<int>(paletteColorFromRgb5(12, 28, 14));
        const int warnTextColor = static_cast<int>(paletteColorFromRgb5(31, 25, 8));
        const int badTextColor = static_cast<int>(paletteColorFromRgb5(31, 10, 10));
        const int accentTextColor = static_cast<int>(paletteColorFromRgb5(20, 27, 31));

        int perfTextColor = goodTextColor;
        if (gFrameP95Ms > 33.0f || gWorstFrame10sMs > 50.0f) {
            perfTextColor = badTextColor;
        } else if (gFrameP95Ms > 20.0f || gWorstFrame10sMs > 33.0f) {
            perfTextColor = warnTextColor;
        }

        if (perfTextColor == goodTextColor) {
            snprintf(headerLine, sizeof(headerLine), "AGME");
        }

        int pacingTextColor = goodTextColor;
        if (gFrameP99Ms > 33.0f || gWorstFrame10sMs > 50.0f) {
            pacingTextColor = badTextColor;
        } else if (gFrameP99Ms > 20.0f || gWorstFrame10sMs > 33.0f) {
            pacingTextColor = warnTextColor;
        }

        int hitchTextColor = goodTextColor;
        if (gWorstFrame10sMs > 50.0f) {
            hitchTextColor = badTextColor;
        } else if (gWorstFrame10sMs > 33.0f) {
            hitchTextColor = warnTextColor;
        }

        int cacheTextColor = warnTextColor;
        if (gCacheHitRate >= 97.0f) {
            cacheTextColor = goodTextColor;
        } else if (gCacheHitRate < 90.0f) {
            cacheTextColor = badTextColor;
        }

        int ioTextColor = goodTextColor;
        if (gIoSlowestReadMs > 24.0f) {
            ioTextColor = badTextColor;
        } else if (gIoSlowestReadMs > 12.0f) {
            ioTextColor = warnTextColor;
        }

        const char* lines[6] = {
            perfLine,
            pacingLine,
            hitchLine,
            cacheLine,
            ioLine,
            loadLine,
        };
        const int lineCount = static_cast<int>(sizeof(lines) / sizeof(lines[0]));
        lineColors[0] = perfTextColor;
        lineColors[1] = pacingTextColor;
        lineColors[2] = hitchTextColor;
        lineColors[3] = cacheTextColor;
        lineColors[4] = ioTextColor;
        lineColors[5] = accentTextColor;
        const int fontHeight = text_height();
        if (fontHeight <= 0) {
            return;
        }

        const int headerHeight = fontHeight + kHudHeaderVerticalPadding * 2;
        const int lineHeight = fontHeight + kHudLineSpacing;
        const int topLineCount = 3;
        const int topLinesHeight = topLineCount > 0 ? topLineCount * lineHeight - kHudLineSpacing : 0;
        const int bottomLineCount = lineCount - topLineCount;
        const int bottomLinesHeight = bottomLineCount > 0 ? bottomLineCount * lineHeight - kHudLineSpacing : 0;

        int width = kHudFixedWidth;
        int height = kHudPadding * 2
            + headerHeight
            + kHudSectionGap
            + topLinesHeight
            + kHudSectionGap
            + kHudSparklineHeight;
        height += kHudSectionGap + bottomLinesHeight;

        width = std::min(width, std::min(kHudMaxWidth, gSdlSurface->w - kHudX));
        height = std::min(height, std::min(kHudMaxHeight, gSdlSurface->h - kHudY));
        if (width <= 6 || height <= 6) {
            return;
        }

        copyRectToBackup(kHudX, kHudY, width, height);

        fillRectIndexed(kHudX, kHudY, width, height, panelBackgroundColor);
        fillRectIndexed(kHudX, kHudY, width, 1, outerBorderColor);
        fillRectIndexed(kHudX, kHudY + height - 1, width, 1, outerBorderColor);
        fillRectIndexed(kHudX, kHudY, 1, height, outerBorderColor);
        fillRectIndexed(kHudX + width - 1, kHudY, 1, height, outerBorderColor);
        fillRectIndexed(kHudX + 1, kHudY + 1, width - 2, 1, innerBorderColor);
        fillRectIndexed(kHudX + 1, kHudY + height - 2, width - 2, 1, innerBorderColor);
        fillRectIndexed(kHudX + 1, kHudY + 1, 1, height - 2, innerBorderColor);
        fillRectIndexed(kHudX + width - 2, kHudY + 1, 1, height - 2, innerBorderColor);

        const int headerX = kHudX + 2;
        const int headerY = kHudY + 2;
        const int headerWidth = width - 4;
        if (headerWidth > 2 && headerHeight > 0) {
            fillRectIndexed(headerX, headerY, headerWidth, headerHeight, headerBackgroundColor);
            fillRectIndexed(headerX, headerY, headerWidth, 1, headerAccentColor);
            unsigned char* headerDst = static_cast<unsigned char*>(gSdlSurface->pixels)
                + (headerY + kHudHeaderVerticalPadding) * gSdlSurface->pitch
                + (kHudX + kHudPadding);
            text_to_buf(headerDst, headerLine, width - kHudPadding * 2, gSdlSurface->pitch, goodTextColor);
        }

        const int textX = kHudX + kHudPadding;
        int lineY = kHudY + kHudPadding + headerHeight + kHudSectionGap;
        for (int index = 0; index < topLineCount; index++) {
            if (lineY + fontHeight >= kHudY + height) {
                break;
            }

            unsigned char* dst = static_cast<unsigned char*>(gSdlSurface->pixels) + lineY * gSdlSurface->pitch + textX;
            text_to_buf(dst, lines[index], width - kHudPadding * 2, gSdlSurface->pitch, lineColors[index]);
            lineY += lineHeight;
        }

        int sparklineY = kHudY + kHudPadding + headerHeight + kHudSectionGap + topLinesHeight + kHudSectionGap;
        const int sparklineWidth = width - kHudPadding * 2;
        if (sparklineWidth > 2 && sparklineY + kHudSparklineHeight < kHudY + height) {
            drawFrameSparkline(textX,
                sparklineY,
                sparklineWidth,
                kHudSparklineHeight,
                panelInnerBackgroundColor,
                sparkGridColor,
                sparkGoodColor,
                sparkWarnColor,
                sparkBadColor);
        }

        lineY = sparklineY + kHudSparklineHeight + kHudSectionGap;
        for (int index = topLineCount; index < lineCount; index++) {
            if (lineY + fontHeight >= kHudY + height) {
                break;
            }

            unsigned char* dst = static_cast<unsigned char*>(gSdlSurface->pixels) + lineY * gSdlSurface->pitch + textX;
            text_to_buf(dst, lines[index], width - kHudPadding * 2, gSdlSurface->pitch, lineColors[index]);
            lineY += lineHeight;
        }

        SDL_Rect rect = { kHudX, kHudY, width, height };
        SDL_BlitSurface(gSdlSurface, &rect, gSdlTextureSurface, &rect);

        restoreRectFromBackup(kHudX, kHudY, width, height);

        gLastHudRect = rect;
        gHudWasDrawn = true;
    }

} // namespace

void diagnostics_init()
{
    if (gDiagnosticsInitialized) {
        return;
    }

    memset(gLoadProfileName, 0, sizeof(gLoadProfileName));
    gPerformanceFrequency = SDL_GetPerformanceFrequency();
    gDiagnosticsMode = applyModeConstraints(gDiagnosticsMode);
    gDiagnosticsInitialized = true;
}

void diagnostics_shutdown()
{
    clearHudArtifactIfNeeded();
    gDiagnosticsInitialized = false;
}

void diagnostics_set_mode(int mode)
{
    diagnostics_init();

    gDiagnosticsMode = applyModeConstraints(mode);
    logModeState();
}

int diagnostics_get_mode()
{
    return gDiagnosticsMode;
}

bool diagnostics_is_hud_enabled()
{
    return (gDiagnosticsMode & DIAGNOSTICS_MODE_HUD) != 0;
}

void diagnostics_toggle_hud()
{
    diagnostics_set_mode(gDiagnosticsMode ^ DIAGNOSTICS_MODE_HUD);
}

void diagnostics_begin_load_profile(const char* name)
{
    diagnostics_init();

    gLoadStartCounter = SDL_GetPerformanceCounter();
    gLoadPhaseCount = 0;

    if (name != NULL) {
        strncpy(gLoadProfileName, name, sizeof(gLoadProfileName) - 1);
        gLoadProfileName[sizeof(gLoadProfileName) - 1] = '\0';
    } else {
        strncpy(gLoadProfileName, "load", sizeof(gLoadProfileName) - 1);
        gLoadProfileName[sizeof(gLoadProfileName) - 1] = '\0';
    }

}

void diagnostics_mark_load_phase(const char* phaseName)
{
    diagnostics_init();

    if (gLoadStartCounter == 0) {
        diagnostics_begin_load_profile("load");
    }

    char sanitizedName[64];
    sanitizePhaseName(phaseName, sanitizedName, sizeof(sanitizedName));
    if (sanitizedName[0] == '\0') {
        strncpy(sanitizedName, "phase", sizeof(sanitizedName) - 1);
        sanitizedName[sizeof(sanitizedName) - 1] = '\0';
    }

    const float elapsedMs = countersToMs(SDL_GetPerformanceCounter() - gLoadStartCounter);

    if (gLoadPhaseCount >= kLoadPhaseCapacity) {
        memmove(gLoadPhases, gLoadPhases + 1, sizeof(gLoadPhases[0]) * (kLoadPhaseCapacity - 1));
        gLoadPhaseCount = kLoadPhaseCapacity - 1;
    }

    LoadPhase& phase = gLoadPhases[gLoadPhaseCount++];
    snprintf(phase.name, sizeof(phase.name), "%s", sanitizedName);
    phase.elapsedMs = elapsedMs;

}

void diagnostics_on_present()
{
    diagnostics_init();

    if (gDiagnosticsMode == DIAGNOSTICS_MODE_OFF) {
        return;
    }

    const Uint64 now = SDL_GetPerformanceCounter();
    updateFrameMetrics(now);
    updateCacheMetrics();
    updateIoMetrics(now);
    emitPeriodicLog(now);
    renderHudOverlay();
}

} // namespace fallout
