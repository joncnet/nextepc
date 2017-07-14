#ifndef __APP_H__
#define __APP_H__

#include "core.h"
#include "core_errno.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

CORE_DECLARE(status_t) app_initialize(char *config_path, char *log_path);
CORE_DECLARE(void) app_terminate(void);

CORE_DECLARE(status_t) app_will_initialize(char *config_path, char *log_path);
CORE_DECLARE(status_t) app_did_initialize(char *config_path, char *log_path);
CORE_DECLARE(void) app_will_terminate(void);
CORE_DECLARE(void) app_did_terminate(void);

CORE_DECLARE(status_t) config_initialize(char *config_path);
CORE_DECLARE(void) config_terminate(void);

CORE_DECLARE(status_t) mme_initialize();
CORE_DECLARE(void) mme_terminate(void);

CORE_DECLARE(status_t) hss_initialize();
CORE_DECLARE(void) hss_terminate(void);

CORE_DECLARE(status_t) sgw_initialize();
CORE_DECLARE(void) sgw_terminate(void);

CORE_DECLARE(status_t) pgw_initialize();
CORE_DECLARE(void) pgw_terminate(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !__NEXTEPC_H__ */