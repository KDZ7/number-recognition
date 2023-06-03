#ifndef _BRAIN_H
#define _BRAIN_H

#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// ***************************** FOR SIGMOIDE FUNCTION ***************************

#define __f(x) (1 / (1 + expl(-x)))
#define __error(x, y) 0.5 * powl(y - x, 2)
#define __grad_b(x, y) (-__f(x) * (1 - __f(x)) * (y - __f(x)))
#define __grad_w(x, y, in) __grad_b(x, y) * in

// *******************************************************************************

#define __randInit srand(time(NULL));
#define __randParam(x) (((2.0 * (x)) * ((long double)random() / RAND_MAX)) - ((x)))


#define New(type, size) (type *)malloc(sizeof(type) * size)
#define Del(instance) { free(instance); instance = NULL; }
#define DelArray2D(instance, size) { for (int i = 0; i < size; i++) { Del(instance[i]); } Del(instance); }

typedef unsigned long long N; // Def ensemble |N
typedef N* pN;
typedef long double R; // Def ensemble |R
typedef R* pR;

static R __X(R* w_, R* in_, N n, R b) {
  R x = 0;
  for (N k = 0; k < n; k++)
    x += w_[k] * in_[k];
  x += b;
  return x;
}

// ************************* NEURON *************************
typedef struct Neuron {
  N num;
  pR in_;
  pR w_;
  R b;
  R x;
  R out;
  R error;
  struct Neuron** prev_;
  void (*init)(struct Neuron*, N);
  void (*set)(struct Neuron*, pR in_, pR w_, R b);
  void (*update)(struct Neuron*);
  void (*calibration)(struct Neuron*, R ref_out, R r, R iteration);
} Neuron, * pNeuron, ** Neurons;

static void _SetNeuron(pNeuron self, pR in_, pR w_, R b) {
  self->in_ = in_;
  self->w_ = w_;
  self->b = b;
  self->x = __X(self->w_, self->in_, self->num, self->b);
  self->out = __f(self->x);
}

static void _UpdateNeuron(pNeuron self) {
  for (N k = 0; k < self->num; k++)
    self->in_[k] = self->prev_[k]->out;
  self->set(self, self->in_, self->w_, self->b);

}

static void _CalibrationNeuron(pNeuron self, R ref_out, R r, R iteration) {

  for (N p = 0; p < self->num; p++)
    for (N k = 0; k < iteration; k++) {
      self->w_[p] += -r * __grad_w(self->x, ref_out, self->in_[p]);
      self->b += -r * __grad_b(self->x, ref_out);
      self->x = __X(self->w_, self->in_, self->num, self->b);
      self->out = __f(self->x);
      printf("\n\t > %d \n", k);
      printf("\n\t Grad( w, b ) <=> Grad( %.10Lf, %.10Lf ) = { %.10Lf, %.10Lf }", self->w_[p], self->b, __grad_w(self->x, ref_out, self->in_[p]), __grad_b(self->x, ref_out));
      printf("\n\t x: %.10Lf", self->x);
      printf("\n\t f(x): %.10Lf\n", self->out);
    }
  self->error = __error(self->out, ref_out);
}

static void _InitNeuron(pNeuron self, N num) {
  self->num = num;
  self->update = &_UpdateNeuron;
  self->set = &_SetNeuron;
  self->calibration = &_CalibrationNeuron;
}



// ******************* NEURAL NETWORK *******************

typedef struct NeuralNetwork {
  N n, m;
  Neurons neurons_;
  void (*init)(struct NeuralNetwork*);
  void (*set)(struct NeuralNetwork*, R(*paramIn)[], R(*paramW)[], R(*paramB)[]);
  void (*update)(struct NeuralNetwork*);
  void (*clear)(struct NeuralNetwork*);
} NeuralNetwork, * pNeuralNetwork;

static void _ClearNeuralNetwork(pNeuralNetwork self) {
  if (self->neurons_ == NULL)
    return;
  for (N i = 1; i < self->n; i++)
    for (N j = 0; j < self->m; j++)
      Del(self->neurons_[i][j].prev_);
  DelArray2D(self->neurons_, self->n);
}

static void _SetNeuralNetwork(pNeuralNetwork self, R(*paramIn)[self->m], R(*paramW)[self->m], R(*paramB)[self->m]) {
  for (N i = 0, k = 0; i < self->n; i++)
    for (N j = 0; j < self->m; j++, k++)
      _SetNeuron(&self->neurons_[i][j], paramIn[i], paramW[k], paramB[i][j]);
}

static void _UpdateNeuralNetwork(pNeuralNetwork self) {
  for (N i = 1; i < self->n; i++)
    for (N j = 0; j < self->m; j++)
      self->neurons_[i][j].update(&self->neurons_[i][j]);
}


static void _InitNeuralNetwork(pNeuralNetwork self) {

  __randInit;

  if (self->neurons_ != NULL)
    self->clear(self);

  // ============== N x M NEURON CREATIONS ==============
  self->neurons_ = New(pNeuron, self->n);
  for (N i = 0; i < self->n; i++)
    self->neurons_[i] = New(Neuron, self->m);
  // ====================================================

  // ========================== NETWORK LINK ASSIGNMENTS  ==========================
  for (N i = 0; i < self->n; i++)
    for (N j = 0; j < self->m; j++) {
      self->neurons_[i][j].prev_ = New(pNeuron, self->m);
      for (N k = 0; k < self->m; k++)
        self->neurons_[i][j].prev_[k] = (i < 1) ? NULL : &self->neurons_[i - 1][k];
      self->neurons_[i][j].init = &_InitNeuron;
      self->neurons_[i][j].init(&self->neurons_[i][j], self->m);
    }
  // ===============================================================================

  self->set = &_SetNeuralNetwork;
  self->update = &_UpdateNeuralNetwork;
  self->clear = &_ClearNeuralNetwork;

}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> LEARNING <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

typedef struct {
  N i, j;
  R value;
} Param;

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ DEBUG @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void brainDbg(pNeuralNetwork self) {
  time_t currentTime = time(NULL);
  struct tm* localTime = localtime(&currentTime);
  printf("\n >>> %02d-%02d %02d:%02d:%02d\n", localTime->tm_mday, localTime->tm_mon + 1, localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
  free(localTime);
  printf("\n");
  for (N i = 0; i < self->n; i++) {
    for (N j = 0; j < self->m; j++)
      printf("%.3Lf ", self->neurons_[i][j].out);
    printf("\n");
  }
}

void backup2D(int lines, int columns, R(*tab)[columns], Param(*param)[columns], const char* filepath) {

  FILE* file = fopen(filepath, "a+");
  if (file == NULL)
    return;
  time_t currentTime = time(NULL);
  struct tm* localTime = localtime(&currentTime);
  fprintf(file, "\n >>> %02d-%02d %02d:%02d:%02d\n", localTime->tm_mday, localTime->tm_mon + 1, localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
  free(localTime);

  if (param != NULL) {
    fprintf(file, "\n {\n");
    for (int i = 0; i < lines; i++) {
      fprintf(file, "\t{ ");
      for (int j = 0; j < columns; j++) {
        fprintf(file, "%Lf", param[i][j].value);
        if (j != columns - 1)
          fprintf(file, ", ");
      }
      if (i != lines - 1)
        fprintf(file, " },\n");
      else
        fprintf(file, " } ");
    }
    fprintf(file, "\n }\n");
  }
  else {
    fprintf(file, "\n {\n");
    for (int i = 0; i < lines; i++) {
      fprintf(file, "\t{ ");
      for (int j = 0; j < columns; j++) {
        fprintf(file, "%Lf", tab[i][j]);
        if (j != columns - 1)
          fprintf(file, ", ");
      }
      if (i != lines - 1)
        fprintf(file, " },\n");
      else
        fprintf(file, " } ");
    }
    fprintf(file, "\n }\n");
  }
  fclose(file);
}
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void _AnalyzeNeuralNetwork(pNeuralNetwork pneuralNetwork, Param p, R ref_out, R ref_learn, N max_count) {
  pneuralNetwork->neurons_[p.i][p.j].calibration(&pneuralNetwork->neurons_[p.i][p.j], ref_out, ref_learn, max_count);
}

#endif
