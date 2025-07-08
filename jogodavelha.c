#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>

#define USERNAME_SIZE 50
#define ROWS 3
#define COLS 3

// === STRUCTS ===
typedef struct {
    char username[USERNAME_SIZE];
    char senha[USERNAME_SIZE];
    int total;
    int wins;
    int draws;
    int losses;
} User;

// === VARIÁVEIS GLOBAIS ===
User  *users = NULL;
int userCount = 0;
int userCapacity = 2;

// === FUNÇÕES ===
void inicializarUsuarios();
void carregarUsuariosDoArquivo();
void salvarUsuariosNoArquivo();
char *lerSenha();
int criarLogin();
int logarUsuario();
void exibirRanking();
char jogarPartidaComLogin(int, int);
char jogarPartidaSemLogin();
void atualizarEstatisticas(int, int, char);

void inicializarTabuleiro(char tabuleiro[ROWS][COLS]);
void exibirTabuleiro(const char tabuleiro[ROWS][COLS]);
bool fazerJogada(char tabuleiro[ROWS][COLS], int, int, char);
bool verificarVitoria(const char tabuleiro[ROWS][COLS], char);
bool verificarEmpate(const char tabuleiro[ROWS][COLS]);

void salvarPartidaEmAndamento(char tabuleiro[ROWS][COLS], int jogador, const char *nomeArquivo);
bool carregarPartidaEmAndamento(char tabuleiro[ROWS][COLS], int *jogador, const char *nomeArquivo);
void continuarPartidaComLogin(char tabuleiro[ROWS][COLS], int jogadorIndex, int p1, int p2);

int main() {
    inicializarUsuarios();
    carregarUsuariosDoArquivo();

    int opcao;

    do {
        printf("\n=== MENU PRINCIPAL ===\n");
        printf("1. Criar login\n2. Jogar com login\n3. Jogar sem login\n4. Ranking\n5. Retomar partida não terminada\n0. Sair\nOpcao: ");
        scanf("%d", &opcao); getchar();

        switch (opcao) {
            case 1: criarLogin(); break;
            case 2: {
                printf("\nJogador X:\n");
                int p1 = logarUsuario();
                if (p1 < 0) break;
                printf("\nJogador O:\n");
                int p2 = logarUsuario();
                if (p2 < 0) break;
                char v = jogarPartidaComLogin(p1, p2);
                atualizarEstatisticas(p1, p2, v);
                break;
            }
            case 3: jogarPartidaSemLogin(); break;
            case 4: exibirRanking(); break;
            case 5: {  // Retomar partida não terminada
                int p1 = logarUsuario();
                if (p1 < 0) break;
                int p2 = logarUsuario();
                if (p2 < 0) break;

                char tabuleiro[ROWS][COLS];
                int jogador;

                // Tenta carregar uma partida em andamento
                if (carregarPartidaEmAndamento(tabuleiro, &jogador, "partida_nao_terminada.txt")) {
                    continuarPartidaComLogin(tabuleiro, jogador, p1, p2);
                } else {
                    printf("Nenhuma partida não terminada encontrada.\n");
                }
                break;
            }
            case 0: salvarUsuariosNoArquivo(); printf("Saindo...\n"); break;
            default: printf("Opcao invalida.\n");
        }
    } while (opcao != 0);

    free(users);
    return 0;
}

int compararUsuarios(const void *a, const void *b) {
    User *userA = (User  *)a;    User *userB = (User   *)b;

    if (userB->wins != userA->wins) {
        return userB->wins - userA->wins; // Ordena por vitórias
    }

    return userB->total - userA->total; // Se vitórias forem iguais, ordena por total de partidas
}

int criarLogin() {
    if (userCount == userCapacity) {
        userCapacity *= 2;
        users = realloc(users, userCapacity * sizeof(User));
    }

    char nome[USERNAME_SIZE];
    printf("Digite um nome de usuario: ");
    fgets(nome, USERNAME_SIZE, stdin);
    nome[strcspn(nome, "\n")] = 0;

    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, nome) == 0) {
            printf("Usuario ja existe.\n");
            return -1;
        }
    }

    strcpy(users[userCount].username, nome);
    printf("Digite a senha: ");
    strcpy(users[userCount].senha, lerSenha());

    users[userCount].wins = 0;
    users[userCount].draws = 0;
    users[userCount].losses = 0;
    users[userCount].total = 0;

    userCount++;
    salvarUsuariosNoArquivo();
    printf("Usuario criado com sucesso!\n");
    return userCount - 1;
}

int logarUsuario() {
    char nome[USERNAME_SIZE];
    char senha[USERNAME_SIZE];

    printf("Usuario: ");
    fgets(nome, USERNAME_SIZE, stdin);
    nome[strcspn(nome, "\n")] = 0;
    printf("Senha: ");
    strcpy(senha, lerSenha());

    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, nome) == 0 && strcmp(users[i].senha, senha) == 0) {
            printf("Login bem-sucedido!\n");
            return i;
        }
    }
    printf("Falha no login.\n");
    return -1;
}

char *lerSenha() {
    char *senha = malloc(USERNAME_SIZE * sizeof(char));
    struct termios oldt, newt;
    int i = 0;
    char c;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while ((c = getchar()) != '\n' && i < USERNAME_SIZE - 1) {
        senha[i++] = c;
        printf("*");
    }
    senha[i] = '\0';
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
    return senha;
}

void atualizarEstatisticas(int p1, int p2, char vencedor) {
    users[p1].total++;
    users[p2].total++;

    if (vencedor == 'X') {
        users[p1].wins++;
        users[p2].losses++;
    } else if (vencedor == 'O') {
        users[p2].wins++;
        users[p1].losses++;
    } else {
        users[p1].draws++;
        users[p2].draws++;
    }
    salvarUsuariosNoArquivo();
}

void exibirRanking() {
    printf("\n=== RANKING ===\n");
    qsort(users, userCount, sizeof(User), compararUsuarios);

    for (int i = 0; i < userCount; i++) {
        printf("%s - Total: %d, Vitorias: %d, Empates: %d, Derrotas: %d\n",
               users[i].username,
               users[i].total,
               users[i].wins,
               users[i].draws,
               users[i].losses);
    }
}

void inicializarUsuarios() {
    users = malloc(userCapacity * sizeof(User));
}

void carregarUsuariosDoArquivo() {
    FILE *f = fopen("arquivo.txt", "r");
    if (!f) {
        printf("Arquivo de usuarios nao encontrado. Criando um novo arquivo...\n");
        salvarUsuariosNoArquivo(); // Cria o arquivo vazio
        return;
    }

    char linha[200];
    while (fgets(linha, sizeof(linha), f)) {
        if (userCount == userCapacity) {
            userCapacity *= 2;
            users = realloc(users, userCapacity * sizeof(User));
        }

        // Usando strtok para dividir a linha de forma correta e processar os dados.
        char *token = strtok(linha, ",");
        strcpy(users[userCount].username, token);

        token = strtok(NULL, ",");
        strcpy(users[userCount].senha, token);

        token = strtok(NULL, ",");
        users[userCount].total = atoi(token);

        token = strtok(NULL, ",");
        users[userCount].wins = atoi(token);

        token = strtok(NULL, ",");
        users[userCount].draws = atoi(token);

        token = strtok(NULL, ";");
        users[userCount].losses = atoi(token);

        userCount++;
    }
    fclose(f);
}

void salvarUsuariosNoArquivo() {
    printf("Salvando usuarios no arquivo...\n"); // DEBUG
    FILE *f = fopen("arquivo.txt", "w");
    if (!f) {
        printf("Erro ao abrir o arquivo para escrita.\n");
        return;
    }

    for (int i = 0; i < userCount; i++) {
        fprintf(f, "%s,%s,%d,%d,%d,%d;\n",
                users[i].username,
                users[i].senha,
                users[i].total,
                users[i].wins,
                users[i].draws,
                users[i].losses);
    }
    fclose(f);
}

char jogarPartidaComLogin(int p1, int p2) {
    char tab[ROWS][COLS];
    inicializarTabuleiro(tab);
    char jogador = 'X';
    char vencedor = 'D'; // 'D' para indicar empate inicialmente

    while (true) {
        exibirTabuleiro(tab);
        int l, c;
        printf("Jogador %c (%s), linha e coluna [0-2]: ", jogador, jogador == 'X' ? users[p1].username : users[p2].username);
        scanf("%d %d", &l, &c);
        getchar(); // para evitar bugs com \n no buffer

        if (fazerJogada(tab, l, c, jogador)) {
            // Salva a partida após cada jogada
            salvarPartidaEmAndamento(tab, jogador == 'X' ? 0 : 1, "partida_nao_terminada.txt");

            if (verificarVitoria(tab, jogador)) {
                exibirTabuleiro(tab);
                vencedor = jogador; // Atualiza o vencedor
                remove("partida_nao_terminada.txt"); // Apaga arquivo após conclusão
                break;
            }
            if (verificarEmpate(tab)) {
                exibirTabuleiro(tab);
                vencedor = 'D'; // Indica empate
                remove("partida_nao_terminada.txt"); // Apaga arquivo após empate
                break;
            }
            jogador = jogador == 'X' ? 'O' : 'X'; // Alterna jogadores
        } else {
            printf("Posicao invalida!\n");
        }
    }
    return vencedor;
}

char jogarPartidaSemLogin() {
    char tab[ROWS][COLS];
    inicializarTabuleiro(tab);
    char jogador = 'X';
    char vencedor = 'D';

    while (true) {
        exibirTabuleiro(tab);
        int l, c;
        printf("Jogador %c, linha e coluna [0-2]: ", jogador);
        scanf("%d %d", &l, &c);
        if (fazerJogada(tab, l, c, jogador)) {
            if (verificarVitoria(tab, jogador)) {
                exibirTabuleiro(tab);
                printf("Jogador %c venceu!\n", jogador);
                vencedor = jogador;
                break;
            }
            if (verificarEmpate(tab)) {
                exibirTabuleiro(tab);
                printf("Empate!\n");
                break;
            }
            jogador = jogador == 'X' ? 'O' : 'X'; // Alterna jogadores
        } else {
            printf("Posicao invalida!\n");
        }
    }
    return vencedor;
}

void inicializarTabuleiro(char tab[ROWS][COLS]) {
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            tab[i][j] = ' ';
}

void exibirTabuleiro(const char tab[ROWS][COLS]) {
    printf("\n");
    for (int i = 0; i < ROWS; i++) {
        printf(" %c | %c | %c \n", tab[i][0], tab[i][1], tab[i][2]);
        if (i < ROWS - 1) printf("--👾---👾--\n");
    }
    printf("\n");
}

bool fazerJogada(char tab[ROWS][COLS], int l, int c, char jogador) {
    if (l >= 0 && l <ROWS && c >= 0 && c < COLS && tab[l][c] == ' ') {
        tab[l][c] = jogador;
        return true;
    }
    return false;
}

bool verificarVitoria(const char tab[ROWS][COLS], char j) {
    // Verifica linhas e colunas
    for (int i = 0; i < ROWS; i++) {
        if ((tab[i][0] == j && tab[i][1] == j && tab[i][2] == j) || 
            (tab[0][i] == j && tab[1][i] == j && tab[2][i] == j)) {
            return true;
        }
    }
    // Verifica diagonais
    if ((tab[0][0] == j && tab[1][1] == j && tab[2][2] == j) || 
        (tab[0][2] == j && tab[1][1] == j && tab[2][0] == j)) {
        return true;
    }
    return false;
}

bool verificarEmpate(const char tab[ROWS][COLS]) {
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            if (tab[i][j] == ' ') return false; // Se houver espaço vazio, não é empate
    return true; // Se não houver espaços vazios, é empate
}

void salvarPartidaEmAndamento(char tabuleiro[ROWS][COLS], int jogador, const char *nomeArquivo) {
    FILE *f = fopen(nomeArquivo, "w");
    if (!f) {
        printf("Erro ao abrir o arquivo para salvar a partida.\n");
        return;
    }

    // Salva o estado do tabuleiro
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            fprintf(f, "%c", tabuleiro[i][j]);
            if (j < COLS - 1) fprintf(f, ",");
        }
        fprintf(f, "\n");
    }

    // Salva o jogador da vez (0 = X, 1 = O)
    fprintf(f, "%d\n", jogador);

    fclose(f);
    printf("Partida salva com sucesso!\n");
}

bool carregarPartidaEmAndamento(char tabuleiro[ROWS][COLS], int *jogador, const char *nomeArquivo) {
    FILE *f = fopen(nomeArquivo, "r");
    if (!f) {
        printf("Arquivo de partida nao encontrado.\n");
        return false;
    }

    // Carrega o estado do tabuleiro
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            fscanf(f, "%c", &tabuleiro[i][j]);
            if (j < COLS - 1) fgetc(f); // Consome a vírgula
        }
        fgetc(f); // Consome a nova linha
    }

    fscanf(f, "%d\n", jogador);  // Lê o jogador da vez
    fclose(f);
    return true;
}

void continuarPartidaComLogin(char tab[ROWS][COLS], int jogadorAtual, int p1, int p2) {
    char jogador = jogadorAtual == 0 ? 'X' : 'O';
    char vencedor = 'D';

    while (true) {
        exibirTabuleiro(tab);
        int l, c;
        printf("Jogador %c (%s), linha e coluna [0-2]: ", jogador, jogador == 'X' ? users[p1].username : users[p2].username);
        scanf("%d %d", &l, &c); getchar();

        if (fazerJogada(tab, l, c, jogador)) {
            salvarPartidaEmAndamento(tab, jogador == 'X' ? 0 : 1, "partida_nao_terminada.txt");

            if (verificarVitoria(tab, jogador)) {
                exibirTabuleiro(tab);
                printf("Jogador %c venceu!\n", jogador);
                vencedor = jogador;
                remove("partida_nao_terminada.txt"); // Apaga arquivo após conclusão
                break;
            }
            if (verificarEmpate(tab)) {
                exibirTabuleiro(tab);
                printf("Empate!\n");
                remove("partida_nao_terminada.txt"); // Apaga arquivo após empate
                break;
            }
            jogador = jogador == 'X' ? 'O' : 'X'; // Alterna jogadores
        } else {
            printf("Posicao invalida!\n");
        }
    }
}