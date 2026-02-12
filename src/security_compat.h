/* Compatibility layer for SELinux vs USEC backends.
 *
 * This keeps libsemanage from directly calling libselinux symbols and
 * allows runtime selection of the security backend.
 */

#ifndef SEMANAGE_SECURITY_COMPAT_H
#define SEMANAGE_SECURITY_COMPAT_H

#include <semanage/handle.h>
#include <selinux/selinux.h>
#include <usec/usec.h>

enum semanage_security_backend semanage_compat_get_backend(const semanage_handle_t *sh);
int semanage_compat_set_backend(semanage_handle_t *sh, enum semanage_security_backend backend);

const char *semanage_compat_security_path(semanage_handle_t *sh);
const char *semanage_compat_security_policy_root(semanage_handle_t *sh);
const char *semanage_compat_security_file_context_path(semanage_handle_t *sh);
const char *semanage_compat_security_file_context_homedir_path(semanage_handle_t *sh);
const char *semanage_compat_security_file_context_local_path(semanage_handle_t *sh);
const char *semanage_compat_security_netfilter_context_path(semanage_handle_t *sh);
const char *semanage_compat_security_usersconf_path(semanage_handle_t *sh);
const char *semanage_compat_security_binary_policy_path(semanage_handle_t *sh);

int semanage_compat_security_set_policy_root(semanage_handle_t *sh, const char *path);
char *semanage_compat_security_boolean_sub(semanage_handle_t *sh, const char *name);

int semanage_compat_security_getenforce(semanage_handle_t *sh);
int semanage_compat_security_get_boolean_names(semanage_handle_t *sh, char ***names, int *len);
int semanage_compat_security_get_boolean_active(semanage_handle_t *sh, const char *name);
int semanage_compat_security_set_boolean_list(semanage_handle_t *sh, int count, SELboolean *boollist, int permanent);

#endif
