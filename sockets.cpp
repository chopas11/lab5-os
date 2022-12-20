// C++ program to find the inverse of Matrix.
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <algorithm>
#include <iterator>
// #include <string.h>
// #include "string"
using namespace std;
#define ADDRESS "gachi_server"
#define BUFFER_LENGTH (2 * order * order * sizeof(float)) + order * sizeof(float)

// Function to Print matrix.
void PrintMatrix(float **ar, int n, int m)
{
  for (int i = 0; i < n; i++)
  {
    for (int j = 0; j < m; j++)
    {
      cout << ar[i][j] << " ";
    }
    printf("\n");
  }
  return;
}

// Function to Print inverse matrix
void PrintInverse(float **ar, int n, int m)
{
  for (int i = 0; i < n; i++)
  {
    for (int j = n; j < m; j++)
    {
      printf("%.3f ", ar[i][j]);
    }
    printf("\n");
  }
  return;
}

// Function to perform the inverse operation on the matrix.
void InverseOfMatrix(float **matrix, int order)
{
  cout << "Sockets" << endl;
  float temp;

  // Выводим исходную матрицу
  printf("Matrix: \n");
  PrintMatrix(matrix, order, order);

  // Создание расширенной матрицы
  for (int i = 0; i < order; i++)
  {
    for (int j = 0; j < 2 * order; j++)
    {
      if (j == (i + order))
        matrix[i][j] = 1;
    }
  }

  // Выводим расширенную матрицу
  printf("\nAugmented Matrix: \n");
  PrintMatrix(matrix, order, order * 2);

  // Заменить строку на сумму самой себя и константы, кратной другой строке матрицы
  for (int i = 0; i < order; i++)
  {
    for (int j = 0; j < order; j++)
    {
      if (j != i)
      {
        temp = matrix[j][i] / matrix[i][i];
        for (int k = 0; k < 2 * order; k++)
        {
          matrix[j][k] -= matrix[i][k] * temp;
        }
      }
    }
  }

  // // Выводим матрицу с ненулевой диагональю
  printf("\nAugmented Matrix with non zero main diagonal: \n");
  PrintMatrix(matrix, order, order * 2);

  ///////////////////////////////
  int sockServ = -1;
  int sd2 = -1;
  sockServ = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sockServ < 0)
  {
    perror("socket creation failed");
    return;
  }
  struct sockaddr_un srvr_name;
  srvr_name.sun_family = AF_UNIX;
  strcpy(srvr_name.sun_path, ADDRESS);
  int err = bind(sockServ, (struct sockaddr *)&srvr_name, SUN_LEN(&srvr_name));
  if (err == -1)
  {
    perror("bind failed");
  }
  err = listen(sockServ, 2);
  if (err == -1)
  {
    perror("listen failed");
  }

  // Выводим информацию о id процессах
  int pid, pid2;
  printf("Main process has pid: %d\n", getpid());
  pid = fork();
  if (pid > 0)
    pid2 = fork();

  if (pid == 0)
    printf("Child process 1 has pid: %d\n", getpid());

  if (pid2 == 0 && pid > 0)
    printf("Child process 2 has pid: %d\n", getpid());

  /////////////////////////// Дочерний процесс 1 //////////////////////////////

  if (pid == 0)
  {
    float oddMatrix[3][7];
    int row = 0;
    oddMatrix[0][0] = 0; // бит, определяющий матрицу как нечетную
    for (int i = 0; i < order; i++)
    {
      if (i % 2 == 0)
      {
        temp = matrix[i][i];
        for (int j = 0; j < 2 * order; j++)
        {
          oddMatrix[row][j + 1] = matrix[i][j] / temp;
        }
        row += 1;
      }
    }

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
    {
      perror("socket creation failed");
      return;
    }
    struct sockaddr_un srvr_name1;
    srvr_name1.sun_family = AF_UNIX;
    strcpy(srvr_name1.sun_path, ADDRESS);
    int err = connect(sock, (struct sockaddr *)&srvr_name1, SUN_LEN(&srvr_name1));
    if (err < 0)
    {
      perror("connect() failed");
      exit(1);
    }
    send(sock, oddMatrix, BUFFER_LENGTH, 0);
    close(sock);
  }
  /////////////////////////// Дочерний процесс 2 //////////////////////////////
  if (pid2 == 0 && pid > 0)
  {
    int row = 0;
    float evenMatrix[3][7];
    evenMatrix[0][0] = 1; // бит, определяющий матрицу как четную
    for (int i = 0; i < order; i++)
    {
      if (i % 2 != 0)
      {
        temp = matrix[i][i];
        for (int j = 0; j < 2 * order; j++)
        {
          evenMatrix[row][j + 1] = matrix[i][j] / temp;
        }
        row += 1;
      }
    }

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
    {
      perror("socket creation failed");
      return;
    }
    struct sockaddr_un srvr_name1;
    srvr_name1.sun_family = AF_UNIX;
    strcpy(srvr_name1.sun_path, ADDRESS);
    int err = connect(sock, (struct sockaddr *)&srvr_name1, SUN_LEN(&srvr_name1));
    if (err < 0)
    {
      perror("connect() failed");
      exit(1);
    }
    send(sock, evenMatrix, BUFFER_LENGTH, 0);
    close(sock);
  }

  ///////////////////////////  Главный процесс  //////////////////////////////
  if (pid > 0 && pid2 > 0)
  {
    sleep(1);
    int bytes, count = 0;
    float matrixOdd[3][7];
    float matrixEven[3][7];
    float matrixRecv[3][7];
    while (count < 2)
    {
      sd2 = accept(sockServ, NULL, NULL);
      if (sd2 < 0)
      {
        perror("accept() failed");
        exit(1);
      }
      bytes = recv(sd2, matrixRecv, BUFFER_LENGTH, 0);
      if (bytes == -1)
      {
        perror("recv");
        break;
      }
      else if (bytes > 0)
      {
        printf("%d bytes received in Main process with pid %d through socket with descriptor %d\n", bytes, getpid(), sd2);
        if (matrixRecv[0][0] == 0)
        {
          for (int i = 0; i < 3; i++)
          {
            for (int j = 0; j < 7; j++)
            {
              matrixOdd[i][j] = matrixRecv[i][j];
            }
          }
        }
        else if (matrixRecv[0][0] == 1)
        {
          for (int i = 0; i < 3; i++)
          {
            for (int j = 0; j < 7; j++)
            {
              matrixEven[i][j] = matrixRecv[i][j];
            }
          }
        }
        count += 1;
      }
    }
    if (sockServ != -1)
      close(sockServ);
    if (sd2 != -1)
      close(sd2);
    unlink(ADDRESS);
    for (int i = 0; i < order; i++)
    {
      for (int j = 1; j < (2 * order) + 1; j++)
      {
        if (i % 2 == 0)
        {
          if (i == 0)
          {
            matrix[i][j - 1] = matrixOdd[i][j];
          }
          else
          {
            matrix[i][j - 1] = matrixOdd[i / 2][j];
          }
        }
        else
        {
          matrix[i][j - 1] = matrixEven[i / 2][j];
        }
      }
    }
    printf("\n Inverse Matrix :\n");
    PrintInverse(matrix, order, 2 * order);
  }
  return;
}

int main()
{
  int order;
  // Порядок матрицы
  order = 2;

  float **matrix = new float *[20];
  for (int i = 0; i < 20; i++)
    matrix[i] = new float[20];

  if (order == 2)
  {
    matrix[0][0] = 4;
    matrix[0][1] = 5;
    matrix[1][0] = 6;
    matrix[1][1] = 8;
  }
  else if (order == 3)
  {
    matrix[0][0] = 1;
    matrix[0][1] = -3;
    matrix[0][2] = 2;
    matrix[1][0] = 0;
    matrix[1][1] = -2;
    matrix[1][2] = 1;
    matrix[2][0] = 3;
    matrix[2][1] = 0;
    matrix[2][2] = 2;
  }
  InverseOfMatrix(matrix, order);
  return 0;
}