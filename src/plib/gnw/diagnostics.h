#ifndef FALLOUT_PLIB_GNW_DIAGNOSTICS_H_
#define FALLOUT_PLIB_GNW_DIAGNOSTICS_H_

namespace fallout {

enum DiagnosticsMode {
    DIAGNOSTICS_MODE_OFF = 0,
    DIAGNOSTICS_MODE_HUD = 1 << 0,
    DIAGNOSTICS_MODE_LOG = 1 << 1,
};

void diagnostics_init();
void diagnostics_shutdown();

void diagnostics_set_mode(int mode);
int diagnostics_get_mode();
bool diagnostics_is_hud_enabled();
bool diagnostics_is_log_enabled();

void diagnostics_toggle_hud();
void diagnostics_toggle_log();

void diagnostics_begin_load_profile(const char* name);
void diagnostics_mark_load_phase(const char* phaseName);

void diagnostics_on_present();

} // namespace fallout

#endif /* FALLOUT_PLIB_GNW_DIAGNOSTICS_H_ */
