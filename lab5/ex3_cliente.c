#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    for (int i = 0; i < 2; i++)
    {

        int request = open("cliente_para_servidor", O_WRONLY);
        if (request < 0)
        {
            perror("erro ao abrir fifo");
        }
        printf("Cliente: consegui abrir a conexão do pedido!\n");

        char buffer[100] = "oi servidor!";

        write(request, buffer, sizeof(buffer));

        close(request);

        printf("Cliente: consegui enviar o pedido!\n");

        int response = open("servidor_para_cliente", O_RDONLY);
        if (response < 0)
        {
            perror("erro ao abrir fifo");
        }

        printf("Cliente: consegui abrir a conexão da resposta!\n");

        ssize_t bytes_read = 0;

        while (bytes_read == 0)
        {
            bytes_read = read(response, buffer, sizeof(buffer));
        }

        close(response);

        printf("Cliente: servidor respondeu isso: %s\n", buffer);
    }

    return 0;
}