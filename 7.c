#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

/* Escuchar servidor, correr este archivo,
para conectar poner telnet 127.0.0.1 8080
probar comando en la consola de telnet "ls",
con el comando "salir" desconectas la conexión
si el addres esta en uso usar " sudo lsof -i :8080" */

#define PORT 8080        
#define BUFFER_SIZE 1024 

void *handle_client(void *arg) {
  int client_socket = *(int *)arg; 
  free(arg);                      
  char buffer[BUFFER_SIZE];

  while (1) {
    memset(buffer, 0, BUFFER_SIZE);
    ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
      perror("Error al recibir datos del cliente");
      break;
    }
    buffer[strcspn(buffer, "\r\n")] = 0;

    if (strcmp(buffer, "salir") == 0) {
      printf("Cliente desconectado.\n");
      break;
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
      perror("Error al crear el pipe");
      close(client_socket);
      pthread_exit(NULL);
    }

    pid_t pid = fork(); 
    if (pid == -1) {
      perror("Error al crear el proceso hijo");
      close(client_socket);
      pthread_exit(NULL);
    } else if (pid == 0) {             
      close(pipe_fd[0]);               
      dup2(pipe_fd[1], STDOUT_FILENO); 
      dup2(pipe_fd[1], STDERR_FILENO); 
      close(pipe_fd[1]);
      execlp("/bin/sh", "sh", "-c", buffer, (char *)NULL);
      perror("Error al ejecutar el comando");
      exit(EXIT_FAILURE);
    } else {             
      close(pipe_fd[1]); 

      char read_buffer[BUFFER_SIZE];
      ssize_t bytes_read;
      while ((bytes_read =
                  read(pipe_fd[0], read_buffer, sizeof(read_buffer) - 1)) > 0) {
        read_buffer[bytes_read] =
            '\0'; 
        send(client_socket, read_buffer, bytes_read,
             0); 
      }

      close(pipe_fd[0]); 
      waitpid(pid, NULL,
              0); }
  }

  close(client_socket);
  pthread_exit(NULL);   
}

int main() {
  // Crear el socket del servidor
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    perror("Error al crear el socket del servidor");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  // Enlazar el socket con el puerto especificado
  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) == -1) {
    perror("Error al enlazar el socket");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  // Escuchar conexiones entrantes
  if (listen(server_socket, 10) == -1) {
    perror("Error al escuchar en el socket");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  printf("Servidor escuchando en el puerto %d...\n", PORT);

  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Aceptar la conexión de un cliente
    int *client_socket = malloc(sizeof(int));
    *client_socket = accept(server_socket, (struct sockaddr *)&client_addr,
                            &client_addr_len);
    if (*client_socket == -1) {
      perror("Error al aceptar la conexión");
      free(client_socket);
      continue;
    }

    printf("Conexión aceptada desde %s:%d\n", inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));


    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_client, client_socket) != 0) {
      perror("Error al crear el hilo");
      close(*client_socket);
      free(client_socket);
      continue;
    }

    pthread_detach(thread_id);
  }

  close(server_socket); 
  return 0;
}
