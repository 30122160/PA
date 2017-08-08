#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);
void set_wp(char *args);
void free_wp(int N );
void print_wp();
/* We use the ``readline'' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args){//args从字符转换成数字  单步执行  
	int steps =1;
	if(args!=NULL){
		steps = atoi(args);	
	}
		cpu_exec(steps);
		return 0;
	
}

int info_r(char *args){
	char *arg = strtok(args,"$");
	int i = 0;
	for(i = R_EAX; i <= R_EDI; i++){
		if(strcmp(arg,regsl[i]) == 0)
			return reg_l(i);
	}
	for(i = R_AX; i <= R_DI; i++){
		if(strcmp(arg,regsw[i]) == 0)
			return reg_w(i);
	}
	for(i = R_AL; i <= R_BH; i++){
		if(strcmp(arg,regsb[i]) == 0)
			return reg_b(i);
	}
	printf("Register error\n");
	return -1;
}

static int cmd_info(char *args){//   打印程序状态 
	 char *arg = strtok(NULL,"$");//去掉字符串中的 $ 符
	 int len=strlen(arg);
	 if(len == 1){
		if(strcmp(arg,"r") == 0){//输出所有寄存器的值
		 	int i = 0;
			for(i = R_EAX; i <= R_EDI; i++){
				printf("%s:\t0x%x\t%d\n",regsl[i],reg_l(i),reg_l(i));
			}
			for(i = R_AX; i <= R_DI; i++){
				printf("%s:\t0x%x\t%d\n",regsw[i],reg_w(i),reg_w(i));
			}
			for(i = R_AL; i <= R_BH; i++){
				printf("%s:\t0x%x\t%d\n",regsb[i],reg_b(i),reg_b(i));
			}
	 	}else if(strcmp(arg,"w") == 0){
			print_wp();
		}
	 }else{//输出某个特定寄存器的值
			int tmp = info_r(args);
			if(tmp == -1)
				return 0;
				printf("%s:\t0x%x\t%d\n",args,tmp,tmp);
	 }
	  return 0;	
}


static int cmd_eval(char *args){//表达式求值 
	bool success = true;
	int num = expr(args,&success);	
	if(success == false)
		return 0;
	printf("%d\n",num);
	return 0;
	
}


static int cmd_x(char *args){//扫描内存 表达式的值作为起始内存地址，输出连续的N个四字节
	char *N = strtok(NULL," ");//获取N值
	char *arg = strtok(NULL," ");//获取表达式
	int i;
	bool success = true;
	int expAdr = expr(arg,&success);//表达式的值
	if(success == false)
		return 0;
	printf("0x%x: ",expAdr);
	for(i = 0; i < atoi(N); i++){
		printf("0x%x\t",swaddr_read(expAdr+i, 1));//输出对应地址内存的值
	}
	printf("\n");
	return 0;	
}

static int cmd_w(char *args){//设置监视点 当表达式的值发生变化时，暂停
	set_wp(args);
	return 0;
}

static int cmd_d(char *args){//删除监视点
	int N=atoi(args);
	free_wp(N);
	return 0;
}

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{"si","Single Step",cmd_si},
	{"info","Print Program Status",cmd_info},
	{"p","Expression evaluation",cmd_eval},
	{"x","Scan the memory",cmd_x},
	{"w","Set the watch point",cmd_w},
	{"d","Delete the watch point",cmd_d},

	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
