/* Compatibility layer for SELinux vs USEC backends. */

#include "security_compat.h"
#include "handle.h"

#include <errno.h>

static enum semanage_security_backend semanage_default_backend =
	SEMANAGE_SECURITY_BACKEND_SELINUX;

static int semanage_compat_backend_supported(enum semanage_security_backend backend)
{
	switch (backend) {
	case SEMANAGE_SECURITY_BACKEND_SELINUX:
		return 1;
	case SEMANAGE_SECURITY_BACKEND_USEC:
		/* USEC is supported if linked at compile time */
		return 1;
	default:
		return 0;
	}
}

enum semanage_security_backend semanage_compat_get_backend(const semanage_handle_t *sh)
{
	if (sh)
		return sh->security_backend;
	return semanage_default_backend;
}

int semanage_compat_set_backend(semanage_handle_t *sh, enum semanage_security_backend backend)
{
	if (!semanage_compat_backend_supported(backend)) {
		errno = ENOTSUP;
		return -1;
	}

	if (sh) {
		sh->security_backend = backend;
	}
	
	semanage_default_backend = backend;

	return 0;
}

static int semanage_compat_use_usec(const semanage_handle_t *sh)
{
	return semanage_compat_get_backend(sh) == SEMANAGE_SECURITY_BACKEND_USEC;
}

const char *semanage_compat_security_path(semanage_handle_t *sh)
{
	if (semanage_compat_use_usec(sh))
		return usec_path();

	return selinux_path(); // /etc/selinux
}

const char *semanage_compat_security_policy_root(semanage_handle_t *sh)
{
	if (semanage_compat_use_usec(sh))
		return usec_policy_root();

	return selinux_policy_root();// /etc/selinux/default
}

const char *semanage_compat_security_file_context_path(semanage_handle_t *sh)
{
	if (semanage_compat_use_usec(sh))
		return usec_file_context_path();

	return selinux_file_context_path();
}

const char *semanage_compat_security_file_context_homedir_path(semanage_handle_t *sh)
{
	if (semanage_compat_use_usec(sh))
		return usec_file_context_homedir_path();

	return selinux_file_context_homedir_path();
}

const char *semanage_compat_security_file_context_local_path(semanage_handle_t *sh)
{
	if (semanage_compat_use_usec(sh))
		return usec_file_context_local_path();

	return selinux_file_context_local_path();
}

const char *semanage_compat_security_netfilter_context_path(semanage_handle_t *sh)
{
	if (semanage_compat_use_usec(sh))
		return usec_netfilter_context_path();

	return selinux_netfilter_context_path();
}

const char *semanage_compat_security_usersconf_path(semanage_handle_t *sh)
{
	if (semanage_compat_use_usec(sh))
		return usec_usersconf_path();

	return selinux_usersconf_path();
}

const char *semanage_compat_security_binary_policy_path(semanage_handle_t *sh)
{
	if (semanage_compat_use_usec(sh))
		return usec_binary_policy_path();

	return selinux_binary_policy_path();
}

int semanage_compat_security_set_policy_root(semanage_handle_t *sh, const char *path)
{
	if (semanage_compat_use_usec(sh))
		return usec_set_policy_root(path);

	return selinux_set_policy_root(path);
}

char *semanage_compat_security_boolean_sub(semanage_handle_t *sh, const char *name)
{
	if (semanage_compat_use_usec(sh))
		return usec_boolean_sub(name);

	return selinux_boolean_sub(name);
}

int semanage_compat_security_getenforce(semanage_handle_t *sh)
{
	if (semanage_compat_use_usec(sh))
		return usec_security_getenforce();

	return security_getenforce();
}

int semanage_compat_security_get_boolean_names(semanage_handle_t *sh, char ***names, int *len)
{
	if (semanage_compat_use_usec(sh))
		return usec_security_get_boolean_names(names, len);

	return security_get_boolean_names(names, len);
}

int semanage_compat_security_get_boolean_active(semanage_handle_t *sh, const char *name)
{
	if (semanage_compat_use_usec(sh))
		return usec_security_get_boolean_active(name);

	return security_get_boolean_active(name);
}

int semanage_compat_security_set_boolean_list(semanage_handle_t *sh, int count, SELboolean *boollist, int permanent)
{
	if (semanage_compat_use_usec(sh))
		return usec_security_set_boolean_list(count, boollist, permanent);

	return security_set_boolean_list(count, boollist, permanent);
}
