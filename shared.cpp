#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cstring>
using namespace std;

// Функция для вывода матрицы
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

// Функция для вывода обратной матрицы
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

// Основная функция для нахождения обратной матрицы
void InverseOfMatrix(float **matrix, int order)
{
  cout << "Shared Memory" << endl;
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

  float matrix1[3][6];
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 6; j++)
    {
      matrix1[i][j] = matrix[i][j];
    }
  }

  // Инициализация shared memory
  int fd = shm_open("/SHAREDMEM", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1)
  {
    perror("open");
    return;
  }
  int res = ftruncate(fd, 2 * order * order * sizeof(float));
  if (res == -1)
  {
    perror("ftruncate");
    return;
  }
  void *addr = mmap(NULL, 2 * order * order * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (addr == MAP_FAILED)
  {
    perror("mmap");
    return;
  }

  //
  memcpy(addr, matrix1, 2 * order * order * sizeof(float));

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
    int fd = shm_open("/SHAREDMEM", O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
      perror("open");
      return;
    }

    addr = mmap(NULL, 2 * order * order * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED)
    {
      perror("mmap");
      return;
    }
    float matrixOdd[3][6];
    memcpy(matrixOdd, addr, 2 * order * order * sizeof(float));
    for (int i = 0; i < order; i++)
    {
      if (i % 2 == 0)
      {
        temp = matrixOdd[i][i];
        for (int j = 0; j < 2 * order; j++)
        {
          matrixOdd[i][j] = matrixOdd[i][j] / temp;
          // cout << "Process ID: " << getpid() << " odd  element [" << i << "][" << j << "] " << matrixOdd[i][j] << "\n";
        }
      }
    }
    memcpy(addr, matrixOdd, 2 * order * order * sizeof(float));
    exit(0);
  }
  /////////////////////////// Дочерний процесс 2 //////////////////////////////
  if (pid2 == 0)
  {
    int fd = shm_open("/SHAREDMEM", O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
      perror("open");
      return;
    }

    addr = mmap(NULL, 2 * order * order * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED)
    {
      perror("mmap");
      return;
    }
    float matrixEven[3][6];
    memcpy(matrixEven, addr, 2 * order * order * sizeof(float));
    for (int i = 0; i < order; i++)
    {
      if (i % 2 != 0)
      {
        temp = matrixEven[i][i];
        for (int j = 0; j < 2 * order; j++)
        {
          matrixEven[i][j] = matrixEven[i][j] / temp;
          // cout << "Process ID: " << getpid() << " even  element [" << i << "][" << j << "] " << matrixEven[i][j] << "\n";
        }
      }
    }
    memcpy(addr, matrixEven, 2 * order * order * sizeof(float));
    exit(0);
  }
  ///////////////////////////////////// Главный процесс /////////////////////////////
  if (pid > 0 && pid2 > 0)
  {
    sleep(3);
    memcpy(matrix1, addr, 2 * order * order * sizeof(float));
    close(fd);
    int res = munmap(addr, 2 * order * order * sizeof(float));
    if (res == -1)
    {
      perror("munmap");
      return;
    }
    fd = shm_unlink("/SHAREDMEM");
    if (fd == -1)
    {
      perror("unlink");
      return;
    }
    for (int i = 0; i < 3; i++)
    {
      for (int j = 0; j < 6; j++)
      {
        matrix[i][j] = matrix1[i][j];
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
    matrix[0][0] = 1;
    matrix[0][1] = 2;
    matrix[1][0] = 3;
    matrix[1][1] = 5;
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