#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 128

typedef struct _Person Person;
typedef void (*func)(Person *);

typedef struct _Person {
    char name[32];
    func greeting;
} Person;

void hello(Person *p) {
    printf("%s says hello.\n", p->name);
}

void yo(Person *p) {
    printf("%s says yo.\n", p->name);
}

void win() {
    printf("flag{insert_flag_here}\n");
}

int main(int argc, char **argv) {
    int stop = 0;
    int greeting_flip = 0;
    char buf[BUF_SIZE];
    Person *p = NULL;

    printf("win() is at %p.\n\n", win);
    printf("Valid commands: \n");
    printf("new: Give birth to a new person.\n");
    printf("greet: Say hello to a person.\n");
    printf("kill: Kill a person.\n");
    printf("quit: Stop the program.\n");
    printf("\n");

    while(!stop) {
        printf("> ");
        fgets(buf, BUF_SIZE, stdin);
        if(strncmp(buf, "new", 3) == 0) {
            p = malloc(sizeof(Person));
            p->greeting = greeting_flip ? hello : yo;
            greeting_flip = !greeting_flip;
            printf("Enter the person\'s name: ");
            fgets(buf, BUF_SIZE, stdin);
            sscanf(buf, "%s", p->name);
            printf("%s was born at %p.\n", p->name, p);
        } else if(strncmp(buf, "greet", 5) == 0) {
            printf("Type in the address of the person you want to say hi to: ");
            fgets(buf, BUF_SIZE, stdin);
            sscanf(buf, "%p", &p);
            printf("You say hello to %s.\n", p->name);
            p->greeting(p);
        } else if(strncmp(buf, "kill", 4) == 0) {
            printf("Type in the address of the person you want to kill: ");
            fgets(buf, BUF_SIZE, stdin);
            sscanf(buf, "%p", &p);
            printf("%s has died.\n", p->name);
            free(p);
        } else if(strncmp(buf, "quit", 4) == 0 || strncmp(buf, "q\n", 2) == 0) {
            printf("Ending program.\n");
            stop = 1;
        } else {
            printf("Invalid command.\n");
        }
    }

    return 0;
}
