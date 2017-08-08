#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;//head指向当前使用的监视点列表的头节点 free 指向空闲的监视点

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

WP* new_wp(){//从 free链表中返回一个空闲的监视点结构 
	if(free_ == NULL) assert(0);
	WP *tmp = free_;
	free_ = free_->next;
	tmp->next = NULL;
	return tmp;
}

void set_wp(char *args){//插入 给监视点赋初始值
	WP *tmp = new_wp();
	strcpy(tmp->args, args);//赋表达式
	bool success = true;
	tmp->num = expr(args,&success);//表达式的值
	if(success == false)
		return;
	if(head == NULL){//将该监视点加入当前监视点队列
		head = tmp;
	}else{
		WP *tmp2 = head;
		while(tmp2->next != NULL){
			tmp2 = tmp2->next;
		}
		tmp2->next = tmp;
	}

}

void free_wp(int N ){//将监视点wp归还到free 链表中
	WP *tmp=head;
	if(tmp == NULL) assert(0);//空表
	if(tmp->NO== N){//头节点是要归还到free链表中的节点
		head = head->next;
		tmp->next = free_;
		free_=tmp;
	}else{
		 while(tmp->next->NO !=N) 
		 	tmp=tmp->next;
		WP *q=tmp->next;
		q->next=free_;
		free_=q;
		tmp->next=q->next;	
	}
}

void print_wp(){//从 head
	if(head !=NULL){
		WP *tmp = head;
		printf("NO     args\n");
		while(tmp != NULL){
			printf("%d\t%s\n",tmp->NO,tmp->args);
			tmp = tmp->next;
		}
	}

}

int check_wp(){//判断表达式是否改变
	if(head != NULL){
		WP *tmp = head;
		while(tmp != NULL){
			bool success = true;
			int num = expr(tmp->args,&success);
			if(num != tmp->num){
				return tmp->NO;
			}
		}
	}
	return -1;
}
/* TODO: Implement the functionality of watchpoint */


