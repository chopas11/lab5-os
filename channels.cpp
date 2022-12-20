#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
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

  // Переменные для дочерних процессов
  int pid, pid2, row;

  int fdEvenIn[2];
  int fdEvenOut[2];
  int fdOddIn[2];
  int fdOddOut[2];

  // Создаем каналы для входа и для выхода
  pipe(fdOddIn);
  pipe(fdOddOut);
  pipe(fdEvenIn);
  pipe(fdEvenOut);
  // Выводим информацию о id процессах
  printf("Main process has pid: %d\n", getpid());
  pid = fork();
  if (pid > 0)
    pid2 = fork();

  if (pid == 0)
    printf("Child process 1 has pid: %d\n", getpid());

  if (pid2 == 0 && pid > 0)
    printf("Child process 2 has pid: %d\n", getpid());

  float matrixOdd[3][6];
  float matrixEven[3][6];
  ///////////////////////////////////// Главный процесс /////////////////////////////
  // Умножаем каждую строку на ненулевое целое число.
  // Делим элемент строки на диагональный элемент
  if (pid > 0 && pid2 > 0)
  {
    for (int i = 0; i < order; i++)
    {
      for (int j = 0; j < 2 * order; j++)
      {
        if (i % 2 == 0)
        {
          if (i == 0)
            matrixOdd[i][j] = matrix[i][j];
          else
            matrixOdd[i - 1][j] = matrix[i][j];
        }
        else
          matrixEven[i - 1][j] = matrix[i][j];
      }
    }
    //
    printf("\n Odd Matrix: \n");
    for (int i = 0; i < order; i++)
    {
      for (int j = 0; j < 2 * order; j++)
      {
        printf("%.3f ", matrixOdd[i][j]);
      }
      printf("\n");
    }
    //
    printf("\n Even Matrix: \n");
    for (int i = 0; i < order; i++)
    {
      for (int j = 0; j < 2 * order; j++)
      {
        printf("%.3f ", matrixEven[i][j]);
      }
      printf("\n");
    }

    //
    write(fdOddIn[1], matrixOdd, 2 * order * order * sizeof(float));
    close(fdOddIn[1]);
    write(fdEvenIn[1], matrixEven, 2 * order * order * sizeof(float));
    close(fdEvenIn[1]);
  }

  row = 0;
  /////////////////////////// Дочерний процесс 1 //////////////////////////////
  if (pid == 0)
  {
    float oddMatrix[3][6];
    read(fdOddIn[0], oddMatrix, 2 * order * order * sizeof(float));
    close(fdOddIn[0]);
    for (int i = 0; i < 2; i++)
    {
      temp = oddMatrix[i][row];

      for (int j = 0; j < 2 * order; j++)
      {
        oddMatrix[i][j] = oddMatrix[i][j] / temp;
      }
      row += 2;
    }
    //
    write(fdOddOut[1], oddMatrix, 2 * order * order * sizeof(float));
    close(fdOddOut[1]);
  }
  /////////////////////////// Дочерний процесс 2 //////////////////////////////
  if (pid2 == 0)
  {
    if (row == 0)
      row = 1;
    float evenMatrix[3][6];
    read(fdEvenIn[0], evenMatrix, 2 * order * order * sizeof(float));
    close(fdEvenIn[0]);
    for (long unsigned int i = 0; i < (sizeof(evenMatrix) / sizeof(evenMatrix[0])); i++)
    {
      temp = evenMatrix[i][row];
      for (int j = 0; j < 2 * order; j++)
      {
        evenMatrix[i][j] = evenMatrix[i][j] / temp;
      }
      row += 2;
    }
    write(fdEvenOut[1], evenMatrix, 2 * order * order * sizeof(float));
    close(fdEvenOut[1]);
  }
  ///////////////////////////  Главный процесс  //////////////////////////////
  if (pid > 0 && pid2 > 0)
  {
    //
    read(fdOddOut[0], matrixOdd, 2 * order * order * sizeof(float));
    close(fdOddOut[0]);
    read(fdEvenOut[0], matrixEven, 2 * order * order * sizeof(float));
    close(fdEvenOut[0]);

    for (int i = 0; i < order; i++)
    {
      if (i % 2 == 0)
      {
        if (i == 0)
          matrix[i] = matrixOdd[i];
        else
          matrix[i] = matrixOdd[i - 1];
      }
      else
        matrix[i] = matrixEven[i - 1];
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
  order = 3;

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