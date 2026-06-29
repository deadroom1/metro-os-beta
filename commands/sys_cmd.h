#ifndef SYS_CMD_H
#define SYS_CMD_H

void cmd_mem(void);
void cmd_time(void);
void cmd_copy(const char *args);
void cmd_cpuinfo(void);
void cmd_shutdown(void);
void cmd_dir(void);
void cmd_cd(const char *args);
void cmd_cat(const char *args);
void cmd_restart(void);
void cmd_echo(const char *args);
void cmd_touch(const char *args);
void cmd_locate(void);
void cmd_uptime(void);
void cmd_countdown(const char *args);
void cmd_banner(const char *args);
const char *cmd_get_current_dir(void);
void cmd_mkdir(const char *args);
void cmd_rmdir(const char *args);
void cmd_rm(const char *args);

#endif
