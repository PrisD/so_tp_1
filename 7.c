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

#define PORT 8080        // Puerto en el que el servidor escucha
#define BUFFER_SIZE 1024 // Tamaño del buffer para recibir y enviar datos

// Función que maneja la comunicación con el cliente
void *handle_client(void *arg) {
  int client_socket = *(int *)arg; // Extraer el socket del cliente
  free(arg);                       // Liberar la memoria del argumento
  char buffer[BUFFER_SIZE];

  while (1) {
    // Limpiar el buffer y recibir el comando del cliente
    memset(buffer, 0, BUFFER_SIZE);
    ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
      perror("Error al recibir datos del cliente");
      break;
    }

    // Remover la nueva línea al final del comando recibido
    buffer[strcspn(buffer, "\r\n")] = 0;

    // Verificar si el comando es "salir" para cerrar la conexión
    if (strcmp(buffer, "salir") == 0) {
      printf("Cliente desconectado.\n");
      break;
    }

    // Crear un pipe para capturar la salida del comando
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
      perror("Error al crear el pipe");
      close(client_socket);
      pthread_exit(NULL);
    }

    pid_t pid = fork(); // Crear un proceso hijo para ejecutar el comando
    if (pid == -1) {
      perror("Error al crear el proceso hijo");
      close(client_socket);
      pthread_exit(NULL);
    } else if (pid == 0) {             // Código del proceso hijo
      close(pipe_fd[0]);               // Cerrar el extremo de lectura del pipe
      dup2(pipe_fd[1], STDOUT_FILENO); // Redirigir la salida estándar al pipe
      dup2(pipe_fd[1], STDERR_FILENO); // Redirigir la salida de error al pipe
      close(pipe_fd[1]);

      // Ejecutar el comando utilizando /bin/sh para interpretar la línea de
      // comando
      execlp("/bin/sh", "sh", "-c", buffer, (char *)NULL);
      perror("Error al ejecutar el comando");
      exit(EXIT_FAILURE);
    } else {             // Código del proceso padre
      close(pipe_fd[1]); // Cerrar el extremo de escritura del pipe

      // Leer la salida del comando desde el pipe y enviarla al cliente
      char read_buffer[BUFFER_SIZE];
      ssize_t bytes_read;
      while ((bytes_read =
                  read(pipe_fd[0], read_buffer, sizeof(read_buffer) - 1)) > 0) {
        read_buffer[bytes_read] =
            '\0'; // Asegurar que la cadena esté correctamente terminada
        send(client_socket, read_buffer, bytes_read,
             0); // Enviar la salida al cliente
      }

      close(pipe_fd[0]); // Cerrar el extremo de lectura del pipe
      waitpid(pid, NULL,
              0); // Esperar a que el proceso hijo termine y limpiar el estado
    }
  }

  close(client_socket); // Cerrar el socket del cliente
  pthread_exit(NULL);   // Terminar el hilo
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

    // Crear un hilo para manejar al cliente
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_client, client_socket) != 0) {
      perror("Error al crear el hilo");
      close(*client_socket);
      free(client_socket);
      continue;
    }

    // Desvincular el hilo para que se maneje automáticamente su memoria
    pthread_detach(thread_id);
  }

  close(server_socket); // Cerrar el socket del servidor
  return 0;
}
