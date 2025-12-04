#include "constants.h"
#include "state.h" // Necessário para SfssRequest e SfssResponse
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE sizeof(SfssRequest) // Tamanho suficiente para a struct
#define FULL_PATH_SIZE (MAX_PATH_LEN + sizeof(SFSS_ROOT) + sizeof("/A0"))

void error(char *msg)
{
    perror(msg);
    exit(1);
}

void init_fs_root()
{
    struct stat st = {0};
    char path[256];

    if (stat(SFSS_ROOT, &st) == -1)
    {
        if (mkdir(SFSS_ROOT, 0700) == 0)
        {
            printf("SFSS: Diretório raiz '%s' criado.\n", SFSS_ROOT);
        }
        else
        {
            perror("SFSS: Erro fatal ao criar diretório raiz");
            exit(1);
        }
    }

    // A0 = Compartilhado, A1..A5 = Processos individuais
    const char *subdirs[] = {"/A0", "/A1", "/A2", "/A3", "/A4", "/A5"};
    int num_subdirs = sizeof(subdirs) / sizeof(subdirs[0]);

    for (int i = 0; i < num_subdirs; i++)
    {
        // Monta o caminho: ./SFSS-root-dir/A1, etc.
        sprintf(path, "%s%s", SFSS_ROOT, subdirs[i]);

        // Se não existir, cria
        if (stat(path, &st) == -1)
        {
            if (mkdir(path, 0700) == 0)
            {
                printf("SFSS: Subdiretório '%s' criado.\n", path);
            }
            else
            {
                perror("SFSS: Erro ao criar subdiretório");
            }
        }
    }
}

void build_full_path(SfssRequest *req, SfssResponse *resp, char *path)
{
    char *process_root_dir;
    // Monta caminho: ./SFSS-root-dir + path vindo do cliente

    // adiciona o diretório raiz do processo
    if (req->args.is_shared)
    {
        process_root_dir = "A0";
    }
    else
    {
        switch (req->process_pos)
        {
        case 0:
            process_root_dir = "A1";
            break;
        case 1:
            process_root_dir = "A2";
            break;
        case 2:
            process_root_dir = "A3";
            break;
        case 3:
            process_root_dir = "A4";
            break;
        case 4:
            process_root_dir = "A5";
            break;
        default:
            // error
            printf("SFSS: process_pos inválido '%d'\n", req->process_pos);
            resp->response.ret_code = ERROR;
            return;
        }
    }

    sprintf(path, "%s/%s", SFSS_ROOT, process_root_dir);
    if (strcmp(req->args.path, "") != 0)
    {
        strcat(path, "/");
        strcat(path, req->args.path);
    }

    resp->response.ret_code = EMPTY;
    return;
}

void handle_read(SfssRequest *req, SfssResponse *resp)
{
    char full_path[FULL_PATH_SIZE];
    build_full_path(req, resp, full_path);
    if (resp->response.ret_code == ERROR)
    {
        return;
    }
    printf("SFSS: Lendo arquivo %s com offset %d\n", full_path, req->args.offset);

    FILE *fp = fopen(full_path, "rb");
    if (fp == NULL)
    {
        resp->response.ret_code = ERROR; // Arquivo não encontrado
        perror("SFSS: Erro ao abrir arquivo");
        return;
    }

    if (fseek(fp, req->args.offset, SEEK_SET) != 0)
    {
        resp->response.ret_code = ERROR;
        fclose(fp);
        return;
    }

    int bytes_read = fread(resp->response.payload, 1, 16, fp);
    if (bytes_read > 0)
    {
        resp->response.ret_code = SUCCESS;
        resp->response.offset = req->args.offset;
        // Limpa o resto do buffer se leu menos que 16
        if (bytes_read < 16)
        {
            memset(resp->response.payload + bytes_read, 0, 16 - bytes_read);
        }
    }
    else
    {
        resp->response.ret_code = ERROR;
    }
    fclose(fp);
}

void handle_write(SfssRequest *req, SfssResponse *resp)
{
    char full_path[FULL_PATH_SIZE];
    build_full_path(req, resp, full_path);
    if (resp->response.ret_code == ERROR)
    {
        return;
    }
    printf("SFSS: Escrevendo arquivo %s com offset %d\n", full_path, req->args.offset);

    FILE *fp = fopen(full_path, "r+b");
    if (fp == NULL)
    {
        fp = fopen(full_path, "w+b");
    }

    if (fp == NULL)
    {
        resp->response.ret_code = ERROR;
        perror("SFSS: Erro ao abrir arquivo");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long current_size = ftell(fp);

    if (req->args.offset > current_size)
    {
        long gap = req->args.offset - current_size;
        for (long i = 0; i < gap; i++)
        {
            fputc(0x20, fp);
        }
    }

    fseek(fp, req->args.offset, SEEK_SET);

    int bytes_written = fwrite(req->args.payload, 1, 16, fp);
    if (bytes_written >= 0)
    {
        resp->response.ret_code = SUCCESS;
        resp->response.offset = req->args.offset;
    }
    else
    {
        resp->response.ret_code = ERROR;
    }
    fclose(fp);
}

void handle_create_directory(SfssRequest *req, SfssResponse *resp)
{
    char full_path[FULL_PATH_SIZE];
    build_full_path(req, resp, full_path);
    if (resp->response.ret_code == ERROR)
    {
        return;
    }

    strcat(full_path, "/");
    strcat(full_path, req->args.dir_name);

    printf("SFSS: Criando diretório %s\n", full_path);

    if (mkdir(full_path, 0700) == 0)
    {
        resp->response.ret_code = SUCCESS;
    }
    else
    {
        perror("SFSS: Erro ao criar diretório");
        resp->response.ret_code = ERROR;
    }
}

void handle_delete(SfssRequest *req, SfssResponse *resp)
{
    char full_path[FULL_PATH_SIZE];
    build_full_path(req, resp, full_path);
    if (resp->response.ret_code == ERROR)
    {
        return;
    }
    printf("SFSS: Deletando arquivo/diretório %s\n", full_path);

    if (remove(full_path) == 0)
    {
        resp->response.ret_code = SUCCESS;
    }
    else
    {
        perror("SFSS: Erro ao deletar arquivo/diretório");
        resp->response.ret_code = ERROR;
    }
}

int file_select(const struct dirent *entry)
{
    if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
        return 0;
    else
        return 1;
}

void handle_list_directory(SfssRequest *req, SfssResponse *resp)
{
    char full_path[FULL_PATH_SIZE];
    build_full_path(req, resp, full_path);
    if (resp->response.ret_code == ERROR)
    {
        return;
    }

    printf("SFSS: Listando diretório %s\n", full_path);

    struct direct **namelist;

    int count = scandir(full_path, &namelist, file_select, alphasort);
    if (count < 0)
    {
        perror("SFSS: Erro ao listar diretório");
        resp->response.ret_code = ERROR;
    }
    else
    {
        resp->response.ret_code = SUCCESS;
        resp->response.nrnames = count;
        int current_position = 0;
        for (int i = 0; i < count; i++)
        {
            strcat(resp->response.allfilenames, namelist[i]->d_name);
            resp->response.fstlstpositions[i].start = current_position;
            resp->response.fstlstpositions[i].end = current_position + strlen(namelist[i]->d_name) - 1;
            resp->response.fstlstpositions[i].type = namelist[i]->d_type == DT_DIR ? TYPE_DIR : TYPE_FILE;
            current_position += strlen(namelist[i]->d_name);
        }
        printf("SFSS: Listei %d arquivos/diretórios: %s\n", count, resp->response.allfilenames);
    }
}

int main(void)
{
    int sockfd;
    struct sockaddr_in serveraddr, clientaddr;
    unsigned int clientlen;
    SfssRequest req;
    SfssResponse resp;
    int n;

    init_fs_root();

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets
     * us rerun the server immediately after we kill it;
     * otherwise we have to wait about 20 secs.
     * Eliminates "ERROR on binding: Address already in use" error.
     */
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

    /*
     * build the server's Internet address
     */
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SFSS_PORT);

    /*
     * bind: associate the parent socket with a port
     */
    if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        error("ERROR on binding");

    printf("SFSS rodando na porta %d...\n", SFSS_PORT);

    /*
     * main loop: wait for a datagram, then process it
     */
    clientlen = sizeof(clientaddr);
    while (1)
    {
        // Recebe SfssRequest
        n = recvfrom(sockfd, &req, sizeof(SfssRequest), 0, (struct sockaddr *)&clientaddr, &clientlen);
        if (n < 0)
            error("ERROR in recvfrom");

        printf("SFSS: REQ recebido do processo %d com Op=%c\n", req.process_pos + 1, req.args.Op);

        // Prepara resposta base
        memset(&resp, 0, sizeof(SfssResponse));
        resp.process_pos = req.process_pos; // Copia o owner ID para devolver

        if (req.args.Op == RD)
        {
            handle_read(&req, &resp);
        }
        else if (req.args.Op == WR)
        {
            handle_write(&req, &resp);
        }
        else if (req.args.Op == DC)
        {
            handle_create_directory(&req, &resp);
        }
        else if (req.args.Op == DL)
        {
            handle_list_directory(&req, &resp);
        }
        else if (req.args.Op == DR)
        {
            handle_delete(&req, &resp);
        }
        // Envia SfssResponse de volta
        n = sendto(sockfd, &resp, sizeof(SfssResponse), 0, (struct sockaddr *)&clientaddr, clientlen);
        if (n < 0)
            error("ERROR in sendto");
    }
}