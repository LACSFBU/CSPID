/****************************************
Testing Block GMRES Method with Deflation

Author: Sehar Naveed
        sehar.naveed@stud-inf.unibz.it
        January 2019

Supervisor: Dr. Bruno Carpentieri
            Dr. Flavio Vella

Institute: Free University of Bolzano
**************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include <lapacke.h>
#include <string.h>
#include <cblas.h>
#include <blas_sparse.h>

#include "clock.h"
#include "coo.h"
#include "csr.h"

unsigned int maxit = 1000;

void get_trans(double *x, double *y, int rows, int cols);
void parse_args(int argc, char *argv[]);
double randf(double low,double high);
void print_matrix(double *arr, int rows, int cols);
double vecnorm( int n, double a1[], double a2[]);
void print_vector(char* pre, double *v, unsigned int size);
double **dmatrix ( int nrl, int nrh, int ncl, int nch );
void matriscopy (double * destmat, double * srcmat, int rowcount, int columncount);
double dot_product(double v[], double u[],  int n);
void subtract(double xx[], double yy[], double result[], int num);

int main (int argc, char * argv[])
{

int rows, rhs;
int M, N, nz, work, lwork;
int initer, iter, i, j, k;
int info,ldb,lda, k_in;
int sym, sym1;

int ret_code;
CSR_Matrix csr;
//CSC_Matrix csc;

int restart, m;
FILE *fp;
char * line = NULL;
size_t len = 0;
ssize_t read;
Clock clock;
MM_typecode matcode;
char* filename;
double *temp;
double *T;
double *Ax;
int *Ap, *Ai;
int istat, stat; 

/* compute sparse matrix matrix multiplication 
*/

parse_args(argc, argv);
 
filename = argv[1];  //Passing on the file 

/***********************************************************
* Reading the Matrix from MM. The matrix is in CSR Format*
************************************************************/

/*TODO:Small tasks: File Management

       1 Change the name from csr to A and the corresponding variables
       2 Create an array with vector tolerance
       3 Shift transpose and print functions to utility and make header file 
       4 Create Residual Matrix with residual block*/

  /********************************************
   * COO to CSR Matrix - Conversion and loading
   * ********************************************/
       printf("\n\nReading Matrix Market file Data\n");
        // Initialize matrices.
       csr_init_matrix(&csr);
  
       // Load matrix from file into COO.
        printf("Loading matrix \"%s\"\n", filename);
       sym = csr_load_matrix(filename, &csr);
  
       if(sym) printf("\n\nMatrix is symmetric\n");
      else    printf("Matrix is general (non-symmetric)\n");
  
       // Print Matrix data
   //      printf("CSR matrix data:\n");
   //    csr_print_matrix(&csr);
 
        printf("\n\nReading : Successful\n");

/********************************
*Initialization 
********************************/
iter = 0;
rows = csr.rows;
rhs = 10;
restart = 10;
m = restart+pd; //In matlab m = inner+pd

//Initialize and allocate B

B = (double*)calloc(rows*rhs, sizeof(double)); //RHS
X = (double*)calloc(rows*rhs, sizeof(double)); //Solution Array
R0 = (double*)calloc(rows*rhs, sizeof(double));//Residual Array

V = (double*)calloc(rows*m, sizeof(double)); //Orthogonal Basis
H = (double*)calloc(m*inner, sizeof(double)); //Hessenberg Matrix
trp = (double*)calloc(rhs*rows, sizeof(double)); //Array for taking transpose of B and Residual


nrm = calloc(rhs, sizeof(double)); 
w = calloc(rows, sizeof(double));  //Allocation of Vector Norm//
y = calloc(rows, sizeof(double));  //Allocation of temperoray vector//
relres = (double *)malloc(rhs* sizeof(double)); // Allocation of Relative Residual//
tau = calloc(rhs,sizeof(double));
scal = calloc(rhs*rhs, sizeof(double));
T = calloc(rhs*rows,sizeof(double));
//V = calloc(rhs*rows, sizeof(double));

E = calloc(m*rhs,sizeof(double));
S = calloc(restart*restart, sizeof(double));

temp = calloc(rows*rhs, sizeof(double));

//TODO: Initialize matrices for SVD

/*******************************
*Generate Random RHS Matrix 
*******************************/

for(i =0; i<rows;i++){
   for (j = 0; j<rhs; j++){
               B[i*rhs+j]= randf(0,1);
    }
}

printf("\n\nThe randomly generated RHS is\n"); 
print_matrix(B,rows,rhs);

/***********************************************************
*Transpose of B/ Calculation Norm and Relative Residual Norm 
************************************************************/

for(i=0; i<rows; ++i){
        for(j=0; j<rhs; ++j){
             T[j*rows+i] = B[i*rhs+j];
         }
}

/*
printf("\n\nThe norm of each RHS is \n");

ldb = rows;
  for (k = 0; k<rhs;k++){
        // for (i = 0; i <rows;i++){
    nrm[k] = vecnorm(rows,&T[k*ldb], &T[k*ldb]);
      if (nrm[k]==0.0){
         nrm[k] = 1.0;
         }
}
 
print_vector("\nnorm =\n ", nrm, rhs);
 printf("\n");
*/

/*************************
*Relative Residual Norm 
**************************/

/*
for (k=0;k<csr.rows; k++){
 e[k] = vecnorm(nrhs, R0[k], R0[k]);
 }
 //Residual Norm 
  for (k=0; k<csr.rows; k++){
  relres[k] = e[k]/w[k];
  }
 print_vector("\n Relative Residual Norm =\n ", relres, csr.rows);
*/


/********************
QR FACTORS 
*********************/


printf("\nQR factorization started\n");

lda = rhs;
info = LAPACKE_dgeqrf(LAPACK_ROW_MAJOR, rows, rhs, B, lda, tau ); //QR Factors 

//print_matrix(B,rows, rhs);


printf("The R factor from QR factorization is\n\n");
for (i=0;i<rhs;i++){
  for(j=0;j<rhs;j++){
             if(i<=j){
                scal[i*rhs+j] = B[i*rhs+j];
     }
  }
}

//print_matrix(scal,rhs,rhs);

/* Extracting V as Q factor of B */

//printf("\n\nThe Q factor is\n");
info = LAPACKE_dorgqr(LAPACK_ROW_MAJOR, rows, rhs, rhs, B, lda, tau);

/*
if (info /= 0){ 
    printf("DQRSL returns info = %d", info);
    exit;
  } 
*/

//print_matrix(B, rows, rhs);
//printf("\n\n The transpose of B--V is \n");

// Allocating the transpose of Q to V//
for (i =0;i<rows; i++){
   for (j=0;j<rhs;j++){
        V[j*rows+i] = B[i*rhs+j];
}
}
//print_matrix(V, rhs, rows);
printf("\n\nQR Factorization, extraction of V and R completed successfully\n");

/*****************************
Block Arnoldi Variant
******************************/
/*Checking matrix vector from Blas and Other Routine*/
/*for (int initer = rhs;initer<m;initer++){
      k_in = initer - rhs;
      csr_mvp_sym2(&csr,&V[k_in*ldb],w);
}

print_vector("\nThe vector w after multiplication from previous routine is  =\n ", w, rows);

for (int initer = rhs;initer<m;initer++){
       k_in = initer - rhs;
       BLAS_dusmv(blas_no_trans, 1.0, A ,&V[k_in*ldb],1, y, 1);
 }

*/

for (int initer = rhs;initer<m;initer++){
         k_in = initer - rhs;
            csr_mvp_sym2(&csr,&V[k_in*ldb],w); //Sparse-Matrix Vector Multiplication 
        /**********************
         Modified Gram-Schmidt 
         **********************/

        
      for (i = 0;i <initer; i++){
                  H[i*restart+k_in]= dot_product(&V[i*ldb], w, rows);
                     //w = w- H[i*restart+k_in]*V[i*ldb];
                     cblas_daxpy (rows,-H[i*restart+k_in], &V[i*ldb],1, w,1);
            }
           H[initer*restart+k_in] = vecnorm(rows,w, w);
          // V[initer*ldb]= w/H[initer*restart+k_in];
          cblas_dscal(rows, 1.0/H[initer*restart+k_in],w, 1);
          cblas_dcopy(rows, w, 1, &V[initer*ldb], 1);  
    /**************
    End of Arnoldi
    ***************/

       /**************
        Givens Rotation
       **************/
            
       //Reading the S matrix from MATLB Source, I need to port this step in C
      // E=[eye(p,p);zeros(k_in,p)]*scal;
      // S = H(1:k_in + p,1:k_in)\E(1:k_in + p,:); 

//Matrix Read      
        fp = fopen("S.txt", "r");//Right Now I am reading S obtained from MATLAB results
        if (fp == NULL)
        exit(0);

        while(!feof(fp)){
              for(i=0;i<restart;i++){
                  for(j=0;j<restart;j++){
                fscanf(fp,"%lf",&S[i*restart+j]);
         }
        }
       } //End of while loop for reading Matrix */

 //Fortran Template
 
// Rotate 
/*for (k = 0; k <k_in;k++){

    r1 = H[k*restart+k_in];
    r2 = H[(k+1)*restart+k_in];
    
   H[k*restart+k_in] = c[k]*r1-s[k]*r2
   H[(k+1)
*/


    /*************************************
    Solution Matrix update and update of R    
    *************************************/
   // X = X0 + V(:,1:k_in)*S;
   //R = B - A * X

//cblas_dgemm (CBLAS_LAYOUT, CBLAS_TRANSPOSE transa, CBLAS_TRANSPOSE transb, m rows of the matrix, n cols of B, k cols of A, alpha, *a, lda,  *b, ldb, beta, *c, ldc);


 cblas_dgemm(CblasRowMajor, CblasTrans, CblasNoTrans, rows, rhs, rhs, 1.0, V, rows, S, rhs, 1.0, X, rhs); 

    get_trans(X, T, csr.rows, rhs);
    csr_mvp_sym2(&csr,&T[k_in*rows],&temp[k_in*rows]);  

//csr_mvp(&csr, X, temp);



    }//End of initer loop 

//print_vector("\nw =\n ", w, rows);
/*
for(j = 0;j <rhs;j++){
   for(i = 0;i <rows;i++){
      w[j] = vecnorm(i, &B[j*rows], &B[j*rows]);
   }  
}*/

/*************************************
Printing Matrix for Debugging
*************************************/
//print_vector("\nThe vector w after multiplication is  =\n ", w, rows);


printf("\n\nThe Matrix S is:\n");
print_matrix(S,restart,restart); 


printf("\n\nThe Orthogonal basis V is:\n");
print_matrix(V,m,rows);


printf("\n\nThe Hessenberg H is:\n");
print_matrix(H,m,restart);

printf("\n\nThe updated Solution X is:\n");
print_matrix(X,rows,rhs);

printf("\n\nThe Residual is:\n");
print_matrix(temp,rows,rhs);

//printf("\n\nThe Identity matrix E is: \n");
//print_matrix(E,m,rhs);



//printf("\n\n");

/******************************
Free Resources
******************************/
free(B);
free(w);
free(y);
free(V);
free(H);
free(E);
free(tau);
free(scal);
free(S);
//BLAS_usds(A);
exit(EXIT_SUCCESS);

} //End of main program 
/****************************************
*Functions 
***************************************/

void print_matrix(double *arr, int rows, int cols){
 
     for(int i = 0; i <rows; i++){
         for (int j=0;j<cols; j++){
 
                printf("\t%e\t",arr[i*cols+j]);

        }
       printf("\n");
     } 
 }

double randf(double low,double high){
return (rand()/(double)(RAND_MAX))*fabs(low-high)+low;
 }

/******************************************************************************/ 
 double vecnorm( int n, double a1[], double a2[])
 /******************************************************************************/
 /*Parameters:
     Input, int N, the number of entries in the vectors.
     Input, double A1[N], A2[N], the two vectors to be considered.
     Output, SQRT of the dot product of the vectors.
 */
 
 {
   int i;
   double value;
 
   value = 0.0;
   for ( i = 0; i < n; i++ ){
     value += a1[i] * a2[i];
   }
   return sqrt(value);
 }


void print_vector(char* pre, double *v, unsigned int size){
        unsigned int i;
       printf("%s", pre);
         for(i = 0; i < size; i++){
//          printf("%.3f\t ", v[i]);
          printf("%e \n", v[i]);
      }
      printf("\t");
  }


      void parse_args(int argc, char *argv[]){
      int i;
      if (argc < 2) {
          printf("Usage: %s input_matrix [iterations]\n", argv[0]);
          exit(EXIT_FAILURE);
      }
        if(argc >= 3){
          i = atoi(argv[2]);
          if (i <= 0){
              printf("Invalid number of iterations.\n");
              exit(EXIT_FAILURE);
          }
              maxit = i;
      }
  }



double **dmatrix ( int nrl, int nrh, int ncl, int nch )
 
 {
   int i;
   double **m;
   int nrow = nrh - nrl + 1;
   int ncol = nch - ncl + 1;
 /* 
   Allocate pointers to the rows.
 */
   m = ( double ** ) malloc ( (size_t) ( ( nrow + 1 ) * sizeof ( double* ) ) );

   if ( ! m )
   {
     fprintf ( stderr, "\n" );
     fprintf ( stderr, "DMATRIX - Fatal error!\n" );
     fprintf ( stderr, "  Failure allocating pointers to rows.\n");
     exit ( 1 );
   }
   m = m + 1;
   m = m - nrl;
 /* 
   Allocate each row and set pointers to them.
 */
   m[nrl] = ( double * ) malloc ( (size_t) ( ( nrow * ncol + 1 ) * sizeof ( double ) ) );
 
   if ( ! m[nrl] )
   {
     fprintf ( stderr, "\n" );
     fprintf ( stderr, "DMATRIX - Fatal error!\n" );
     fprintf ( stderr, "  Failure allocating rows.\n");
     exit ( 1 );
   }
   m[nrl] = m[nrl] + 1;
 /* 
   Return the pointer to the array of pointers to the rows;
 */
   return m;
 }


void matriscopy (double * destmat, double * srcmat, int rowcount, int columncount)
{
  int i, j;
  double (*dst)[columncount];
  double (*src)[columncount];
  dst = (double (*)[columncount])destmat;
  src = (double (*)[columncount])srcmat;
  for (j=0; j<columncount; j++) /* rad-nr */
    for (i=0; i<rowcount; i++) /* kolumn-nr */
      dst[j][i] = src[j][i];
}

double dot_product(double v[], double u[], int n)
{
    double result = 0.0;
    for (int i = 0; i < n; i++)
        result += v[i]*u[i];
    return result;
}

void subtract(double xx[], double yy[], double result[], int num) {
    for (int ii = 0; ii < num; ii++) {
        result[ii] = xx[ii]-yy[ii];
    }
}

void get_trans(double *x, double *y, int rows, int cols){

unsigned int i,j;
for(i=0; i<rows; ++i){
         for(j=0; j<cols; ++j){
              y[j*rows+i] = x[i*cols+j];
          }
 } 
}
