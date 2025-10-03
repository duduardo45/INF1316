#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>

char *to_upper_str(char *string, int n)
{
    for (int i = 0; i < n; i++)
    {
        string[i] = toupper(string[i]);
    }
}

int main(int argc, char *argv[])
{

    while (1)
    {
        int request = open("cliente_para_servidor", O_RDONLY);
        if (request < 0)
        {
            perror("erro ao abrir fifo");
        }

        printf("Servidor: abri a conexão do pedido!\n");

        ssize_t bytes_read = 0;
        char buffer[100];

        while (bytes_read == 0)
        {
            bytes_read = read(request, buffer, sizeof(buffer));
        }

        close(request);

        printf("Servidor: li o pedido: %s\n", buffer);

        int response = open("servidor_para_cliente", O_WRONLY);
        if (response < 0)
        {
            perror("erro ao abrir fifo");
        }

        printf("Servidor: abri a conexão da resposta!\n");

        to_upper_str(buffer, bytes_read);

        write(response, buffer, sizeof(buffer));
        close(response);

        printf("Servidor: escrevi a resposta!\n");
    }

    return 0;
}