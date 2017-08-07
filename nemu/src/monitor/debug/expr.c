#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#define MAX_SIZE 32
uint32_t swaddr_read(swaddr_t addr, size_t len);
int info_r(char *args);

enum {
	NOTYPE = 256, LBracket,VAR,NUMBER,EQ,NEQ,AND,OR,NOT,POINTER,PLUS,MINUS,MULTIPLY,UNARYMINUS,DIV,RBracket


	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules. 根据优先级来排序
	 */
	{" +",	NOTYPE},				// spaces
	{"\\(",LBracket},               //(
	{"\\$[a-z]+",VAR},               //variable
	{"^[0-9]+|0x[a-f0-9]+",NUMBER}, //digit
	{"==", EQ},						// equal
	{"!=",NEQ},                     //not equal
	{"&&",AND},                     //AND
	{"\\|\\|",OR},                  //OR
	{"\\!",NOT},                    //NOT	
	{"\\+", PLUS},					// plus
	{"\\-",MINUS},				    //minus
	{"\\*",MULTIPLY},               //multiply
	{"\\/",DIV},                    //div
	{"\\)",RBracket}                //)
	
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[MAX_SIZE];// 存储优先级等
} Token;

Token tokens[MAX_SIZE];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;//匹配数

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array ``tokens''. For certain 
				 * types of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case NOTYPE:
						break;
					case VAR:
					case NUMBER:
						tokens[nr_token].type=rules[i].token_type;
						if(substr_len>MAX_SIZE)
							assert(0);
						int j;
						for(j = 0; j < substr_len; j++)
							tokens[nr_token].str[j] = substr_start[j];
						tokens[nr_token].str[j] = '\0';
						nr_token++;
						break;
					case OR:
						tokens[nr_token].str[0] = '0';
						tokens[nr_token++].type = rules[i].token_type;
						break;
					case AND:
						tokens[nr_token].str[0] = '1';
						tokens[nr_token++].type = rules[i].token_type;
						break;
					case EQ:
					case NEQ:					
						tokens[nr_token].str[0] = '2';
						tokens[nr_token++].type = rules[i].token_type;
						break;					
					case PLUS:									
					case MINUS:
						tokens[nr_token].str[0] = '3';
						tokens[nr_token++].type = rules[i].token_type;
						break;		
					case MULTIPLY:
					case DIV:
						tokens[nr_token].str[0] = '4';
						tokens[nr_token++].type = rules[i].token_type;
						break;
					case NOT:
					case POINTER:
						tokens[nr_token].str[0] = '5';
						tokens[nr_token++].type = rules[i].token_type;
						break;
					case UNARYMINUS:
						tokens[nr_token].str[0] = '6';
					case LBracket:
					case RBracket:
						tokens[nr_token++].type = rules[i].token_type;
						break;
					default: panic("please implement me");
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}
	return true; 
}

static int stack[MAX_SIZE];
unsigned int StackLen = 0;
static void InitStack(){//初始化
	StackLen = 0;
}
static void push(int var){//入栈
	if(StackLen == MAX_SIZE){
		printf("Stack is full!\n");
		return;
	}
	stack[StackLen++] = var;
}
static void pop(){//出栈
	if(StackLen == 0){
		printf("Stack is empty\n");
		return ;
	}	
	StackLen--;
}
static bool check_parentheses(int start, int end){// 判断最外层是否有括号
	InitStack();
	int i;
	if(tokens[start].type != LBracket || tokens[end].type != RBracket)
		return false;
	for(i = start; i <= end; i++){
		if(tokens[i].type == LBracket)
			push(tokens[i].type);
		if(tokens[i].type == RBracket){
			if(StackLen == 1&&i != end)
				return false;
			pop();
		}
	}
	return true;
}


static bool check_bar(){//查看表达式括号匹配
	InitStack();
	int i;
	for(i = 0; i < nr_token; i++){
		if(tokens[i].type == LBracket)
			push(tokens[i].type);
		else if(tokens[i].type == RBracket){
			if(StackLen == 0)
				return false;
			pop();
		}
	}
	if(StackLen != 0)
		return false;
	return true;
}

static bool is_operator(int type){
	switch(type){
		case PLUS:
		case MINUS:
		case MULTIPLY:
		case DIV:
		case EQ:
		case NEQ:
		case AND:
		case OR:
		case NOT:
		case POINTER:
		case UNARYMINUS:
			return true;
		default:
			return false;
	}
}

static int dominant_op(int start, int end){//dominant operator
	int pos = 0;
	int dominant = 10;
	int i;
	for(i = start; i <= end; i++){
		if(tokens[i].type == LBracket){
			while(tokens[i].type != RBracket)
				i++;
		}
		if(is_operator(tokens[i].type) && tokens[i].str[0]-'0' <= dominant){
			pos = i;
			dominant = tokens[i].str[0]-'0';
		}
	}
	return pos;
}

static int my_atoi(char *args){//16进制数转换为10进制数 
	int i;
	int sum = 0;
	for(i = 2;i < strlen(args); i++){
		if(args[i] >= '0' && args[i] <= '9'){
			sum = sum*16 + args[i]-'0';
		}
		if(args[i] >= 'a' && args[i] <= 'f'){
			sum = sum*16 + args[i]-87;
		}
	}
	return sum;
}

static int eval(int start, int end){
	if(start > end){
		assert(0);
	}
	else if(start == end){
		if(tokens[start].str[0] == '$'){//变量
			return info_r(tokens[start].str);
		}
		else if(strlen(tokens[start].str) > 1 && tokens[start].str[0] == '0' && tokens[start].str[1] == 'x'){//16进制数
			return my_atoi(tokens[start].str);
		}
		return atoi(tokens[start].str);//10进制数
	}
	else if(check_parentheses(start, end)){//去除最外层括号 然后进行计算
		return eval(start+1, end-1);
	}
	else{
		int op = dominant_op(start, end);//找出 dominant operator
		if(tokens[op].type == NOT || tokens[op].type == POINTER || tokens[op].type == UNARYMINUS){//一元操作符
			int val = eval(op+1,end);
			if(tokens[op].type == NOT){
				return !val;
			}			
			else if(tokens[op].type == UNARYMINUS){				
				return -val;
			}
			else{
				return swaddr_read(val,1);//指针解引用
			}				
		}
		int val1 = eval(start, op-1);//分成两部分
		int val2 = eval(op+1, end);
		switch(tokens[op].type){
			case PLUS:return val1 + val2;
			case MINUS:return val1 - val2;
			case MULTIPLY: return val1 * val2;
			case EQ: return val1 == val2;
			case NEQ: return val1 != val2;
			case AND: return val1 && val2;
			case OR: return val1 || val2;
			case DIV:
				assert(val2);
				return val1 / val2;
			default:assert(0);
		}
	}
}

int expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	if(!check_bar()){
		*success = false;
		printf("Bars match error\n");
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */
	int i;
	for(i = 0; i < nr_token; i++){
		if(tokens[i].type == MULTIPLY && (i == 0 || is_operator(tokens[i-1].type)))
			tokens[i].type = POINTER;
		if(tokens[i].type == MINUS && (i == 0 || is_operator(tokens[i-1].type)))
			tokens[i].type = UNARYMINUS;
}
	//panic("please implement me");
	return eval(0,nr_token-1);
}

