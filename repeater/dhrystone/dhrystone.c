#include <shell/shell.h>

int dhry_main(const struct shell *shell, size_t argc, char **argv)
{
	dhrystone_main(argc, argv);
	return 0;
}

SHELL_CREATE_STATIC_SUBCMD_SET(dhry_commands) {
	SHELL_CMD(NULL, NULL,
	 NULL,
	 dhry_main),
	SHELL_SUBCMD_SET_END
};

SHELL_CMD_REGISTER(dhrystone, &dhry_commands, "dhrystone benchmark", dhry_main);

