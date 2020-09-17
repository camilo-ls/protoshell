/* PROTO-SHELL CRIADO POR CAMILO LIRA SIDOU
 * para a matéria de Sistemas Operacionais
 * ministrada pelo prof. André Carvalho
 * Engenharia de Software - iComp - UFAM
 * 01/09/2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// INPUT_DELIM: caracteres a serem retirados pelo strtok;
// INPUT_SIZE: tamanho max de um input do user;
// MAX_ARG: num max. de argumentos que podem ser passados;
// NUM_INT_FUNC: numero de funcoes internas do shell.
#define INPUT_DELIM " \n"
#define INPUT_SIZE 128
#define MAX_ARG 10
#define NUM_INT_FUNC 3

// array de funções nativas do shell
// PS: 'exit' teve implementação particular
char* int_func[] = {"help", "pwd", "cd"};

// declarações antecipadas das funções internas do shell:
int int_help(char** args);
int int_pwd(char** args);
int int_cd(char** args);

// função-ponteiro que executa as funções nativas do shell:
int (*int_func_exec[]) (char** args) = {
	&int_help,
	&int_pwd,
	&int_cd
};

// declarações antecipadas das funções utilizadas:
int cout_intro();
int read_input(char* usr_input);
int parse_input(char* usr_input, char** parsd_input);
int parse_input_piped(char* usr_input, char** usr_input_piped);
int exec_prog(char** args);
int exec_prog_piped(char** args1, char** args2);


// desativei essa função pois estava dando erro:
// free(): double free detected in tcache2
// irei investigar a causa
int clear_buff(char** array, int col);

int main() {
	char *usr_input; // input do user
	char** usr_input_piped; // input do user dividido se houver pipe
	char** parsd_input; // input depois do parse
	char** parsd_input_2; // input depois do parse do 2º comando 
	char *buf; // bufer utilizado para o strtok;
	int status = 1; // 1 = programa rodando / 0 = fim do programa
	int p_pid, wid; // guardam os retornos do fork e wait, respectiv.
	
	// alocações dinâmicas dos arrays declarados acima:
	usr_input = malloc(INPUT_SIZE * sizeof(char));
	
	usr_input_piped = malloc(MAX_ARG * sizeof(char*));
	for (int i = 0; i < MAX_ARG; i++) {
		usr_input_piped[i] = malloc(INPUT_SIZE * sizeof(char));
	}
	
	parsd_input = malloc(MAX_ARG * sizeof(char*));
	for (int i = 0; i < MAX_ARG; i++) {
		parsd_input[i] = malloc(INPUT_SIZE * sizeof(char));
	}
	
	parsd_input_2 = malloc(MAX_ARG * sizeof(char*));
	for (int i = 0; i < MAX_ARG; i++) {
		parsd_input_2[i] = malloc(INPUT_SIZE * sizeof(char));
	}
	
	// imprime a intro. na tela:
	cout_intro();
	
	// loop principal de execução do programa:
	while (status) {
		printf("> ");
		read_input(usr_input);
		parse_input_piped(usr_input, usr_input_piped);
		parse_input(usr_input_piped[0], parsd_input);
		
		// verif. do comando 'exit':
		if (parsd_input[0] != NULL) {	
			if (!strcmp(parsd_input[0], "exit")) {
				status = 0;
				free(usr_input);
				//clear_buff(parsd_input, MAX_ARG);
				exit(0);
			}
		}
		
		// verif. se o comando é piped:
		if (usr_input_piped[1] != NULL) {
			parse_input(usr_input_piped[1], parsd_input_2);
			exec_prog_piped(parsd_input, parsd_input_2);
		}
		// senao é piped, roda o comando unico:
		else if (parsd_input[0] != NULL)
			exec_prog(parsd_input);
	}	
	return 0;
}

// funcao que imprime a intro. do shell:
int cout_intro() {
	printf("==================================================\n");
	printf("TERMINAL ALPHA VERSÃO 0.1 INICIALIZADO\n");
	printf("criado por Camilo Lira Sidou para a materia de SO\n");
	printf("2 de Setembro de 2019\n");	
	printf("==================================================\n");
	printf("\n");
	printf("Digite 'help' para listar os comandos disponiveis!\n");
	printf("\n");
	return 1;	
}

// funcao que lê o input do user e guarda na var. usr_input
int read_input(char* usr_input) {
	fgets(usr_input, INPUT_SIZE, stdin);
	return 1;
}

// faz o parse (divisao) do input e salva em parsd_input
int parse_input(char* usr_input, char** parsd_input) {
	char* buf;
	int i = 0;
	
	buf = strtok(usr_input, INPUT_DELIM);
	while (buf != NULL) {
		parsd_input[i++] = buf;
		buf = strtok(NULL, INPUT_DELIM);
	}
	while (i < MAX_ARG) parsd_input[i++] = NULL;
	return 1;
}

/* divide o input do user de acordo com o pipe, por exemplo:
 * input: pipe1 | pipe2
 * pipe1 sera guardado no indice 0 de usr_input_piped
 * pipe2 sera guardado no indice 1 de usr_input_piped
 */
int parse_input_piped(char* usr_input, char** usr_input_piped) {
	int p_pid = -1;
	int exec_r = -1;
	int i = 0;
	char* buf;
	
	usr_input_piped[1] = NULL;
	
	buf = strtok(usr_input, "|");
	while (buf != NULL) {
		usr_input_piped[i++] = buf;
		buf = strtok(NULL, "|");
	}
	while (i < MAX_ARG) usr_input_piped[i++] = NULL;
	if (usr_input_piped[1] == NULL) return 0;
	return 1;
}

// executa um programa se nao houver pipe:
int exec_prog(char** args) {
	int p_pid = -1;
	int exec_r;
	int in, out;
	
	// checa se é uma funcao interna do shell e a executa se o for:
	for (int i = 0; i < NUM_INT_FUNC; i++) {
		if (strstr(args[0], int_func[i]) != 0) {
			return (*int_func_exec[i])(args);
		}		
	}
	
	p_pid = fork();
	
	if (p_pid < 0) {
		printf("\n>> ERRO: O fork falhou. Abortando.\n\n");
		exit(1);
	}
	else if (p_pid == 0) {
		
		// checa se existe redirecionamento de input/output e abre
		// os streams de in e out, bem como os arquivos, de acordo
		// com o caso:
		if (args[1] != NULL) {
			if (!strcmp(args[1], "<")) {
				in = open(args[2], O_RDONLY);
				if (in < 0) {
					printf("\n>>ERRO: arquivo nao encontrado\n\n");
					close(in);
					exit(1);
				}
				dup2(in, 0);
				close(in);
			}
			if (!strcmp(args[1], ">")) {
				out = open(args[2], O_WRONLY | O_CREAT, 0666);
				if (out < 0) {
					printf("\n>>ERRO: nao foi possivel criar o arquivo\n\n");
					close(out);
					exit(1);
				}
				dup2(out, 1);
				close(out);			
			}
		}
		// execucao de fato do programa:
		exec_r = execvp(args[0], args);
		if (exec_r < 0 )  {
			printf("\n>> ERRO: comando desconhecido\n\n");
			exit(1);
		}
	}
	else wait(NULL);
	return 1;
}

// faz a execucao dos comandos caso haja pipe, onde
// args1 e args2 são vetores de *strings*, cada um
// contendo as linhas de comando divididas, com cada
// parametro em um índice:
int exec_prog_piped(char** args1, char** args2) {
	int pipe_io[2];
	int p_pid1, p_pid2;
	
	// checa se é uma funcao interna do shell e a executa se o for:
	for (int i = 0; i < NUM_INT_FUNC; i++) {
		if (strstr(args1[0], int_func[i]) != 0) {
			return (*int_func_exec[i])(args1);
		}		
	}
	
	// inicializa o pipe:
	if (pipe(pipe_io) < 0) {
		printf("\n>> ERRO: pipe nao executado\n\n");
	}
	
	// fork do primeiro processo (pipe1):
	p_pid1 = fork();
	
	if (p_pid1 < 0) {
		printf("\n>> ERRO: O fork falhou. Abortando.\n\n");
		return -1;
	}
	
	// fecha o input do filho pipe1, mantendo o output
	// processo 1: escreve
	if (p_pid1 == 0) {
		close(pipe_io[0]);
		dup2(pipe_io[1], 1);
		close(pipe_io[1]);
		
		if (execvp(args1[0], args1) < 0) {
			printf("\n>> ERRO: argumento <pipe1> invalido\n");
			exit(1);		
		}		
	}
	else {
		
		// fork do segundo processo (pipe2):
		p_pid2 = fork();
		
		if (p_pid2 < 0) {
			printf("\n>> ERRO: O fork falhou. Abortando.\n\n");
			return -1;
		}
		
		// fecha o output do filho pipe2, mantendo o input
		// processo 2: lê
		if (p_pid2 == 0) {
			close(pipe_io[1]);
			dup2(pipe_io[0], 0);
			close(pipe_io[0]);
			
			if (execvp(args2[0], args2) < 0) {
				printf("\n>> ERRO: argumento <pipe2> invalido\n\n");
				exit(1);	
			}
		}
		else {
			// processo pai: aguarda a finalização de ambos os filhos
			wait(NULL);
			wait(NULL);
		}
	}	
	return 1;
}

// função que imprime os comandos nativos do shell:
int int_help(char** args) {
	printf("\n");
	printf(">> Comandos nativos do shell:\n");
	printf("\n");
	printf(" exit\n");
	for (int i = 0; i < NUM_INT_FUNC; i++) {
		printf(" %s\n", int_func[i]);
	}
	printf("\n");
	return 1;
}

// função que mostra o diretorio corrente:
int int_pwd(char** args) {
	char current_dir[300];
	getcwd(current_dir, 300);
	printf("\n");
	printf("%s\n", current_dir);
	printf("\n");
	return 1;
}

// função para mudar o diretorio corrente:
int int_cd(char** args) {
	int chdir_r;
	
	if (args[1] == NULL) {
		printf("\n>> ERRO: sintaxe do comando: cd <diretorio>\n\n");
		return -1;
	}
	
	strtok(args[1], "\n");	
	chdir_r = chdir(args[1]);
	
	if (chdir_r < 0) {
		printf("\n>> ERRO: diretorio invalido\n\n");
	}
	else {
		printf("\n>> Novo diretorio:");
		int_pwd(NULL);
	}
	
	return 1;
}

// esse comando serviria para desalocar as arrays alocadas dinamicamente
// utilizando a função free(), porém ocorreu o seguinte erro:
// free(): double free detected in tcache2
// não achei informação sobre esse erro na internet, e chequei
// se não estava dando free duas vezes nas arrays, portanto deixarei
// desativada até entender exatamente qual foi o erro
int clear_buff(char** array, int lin) {
	for (int i = 0; i < lin; i++) free(array[i]);
	return 1;
}

