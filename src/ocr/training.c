#include "training.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
#include <err.h>

#define INPUT_SIZE 784
#define HIDDEN_SIZE 32
#define OUTPUT_SIZE 26
#define LEARNING_RATE 0.1

float input[INPUT_SIZE];
float hidden[HIDDEN_SIZE];
float output[OUTPUT_SIZE];

float wIH[INPUT_SIZE][HIDDEN_SIZE]; // poids input-hidden
float bH[HIDDEN_SIZE];              // biais hidden

float wHO[HIDDEN_SIZE][OUTPUT_SIZE]; // poids hidden-output
float bO[OUTPUT_SIZE];               // biais output


//fonction sigmoid
float sigmoid(float x)
{
    return 1.0/(1.0+expf(-x));
}


//fonction softmax
void softmax(float *input, float *output, size_t len)
{
    float sum=0;
    for(size_t i=0;i<len; i++)
    {
        float e=expf(input[i]);
        sum+=e; //calcul sum
        output[i]=e; // copie de input dans output en appliquant exp
    }


    for (size_t i=0;i<len;i++)
    {
        output[i]/=sum;  //calcul de softmax
    }

}

//calcul un poids random
float random_weight()
{
    return ((float)rand() / RAND_MAX) - 0.5;
}


// initialise les poids et biais aléatoirement pour commencer
void init_weights()
{
    for(int i=0;i<INPUT_SIZE;i++)
    {
        for(int j=0;j<HIDDEN_SIZE;j++)
        {
            wIH[i][j]=random_weight();
        }
    }

    for(int i=0;i<HIDDEN_SIZE;i++)
    {
        for(int j=0;j<OUTPUT_SIZE;j++)
        {
            wHO[i][j]=random_weight();
        }
    }

    for(int i=0;i<HIDDEN_SIZE;i++)
    {
        bH[i]=random_weight();
    }

    for(int i=0;i<OUTPUT_SIZE;i++)
    {
        bO[i]=random_weight();
    }

}


//calcul la couche cachée
void calcul_hidden()
{
    for(int i=0;i<HIDDEN_SIZE;i++)
    {
        float tot=bH[i];
        for(int j=0;j<INPUT_SIZE;j++)
        {
            tot+=wIH[j][i]*input[j];
        }
        hidden[i]=sigmoid(tot);
    }
}


//calcul la couche de sortie
void calcul_output()
{
    float temp[OUTPUT_SIZE];
    for(int i=0;i<OUTPUT_SIZE;i++)
    {
        float tot=bO[i];
        for(int j=0;j<HIDDEN_SIZE;j++)
        {
            tot+=wHO[j][i]*hidden[j];
        }
        temp[i]=tot;
    }
    softmax(temp,output,OUTPUT_SIZE);

}


//lance le réseau de neurone et fait les calculs dans les différentes couches
void forward()
{
    calcul_hidden();
    calcul_output();
}



//calcul erreur de la couche de sortie
void calcul_errorO(float *errorO,int index_letter)
{
    for(int i=0;i<OUTPUT_SIZE;i++)
    {
        errorO[i]=output[i]-(i==index_letter?1:0);
    }
    
}



//calcul erreur de la couche cachée
void calcul_errorH(float *errorH,float *errorO)
{
    for(int i=0;i<HIDDEN_SIZE;i++)
    {
        float new_value=0;
        for (int j=0;j<OUTPUT_SIZE;j++)
        {
            new_value+=errorO[j]*wHO[i][j];
        }
        errorH[i]=new_value*hidden[i]*(1-hidden[i]);
    }
    
}


//met à jour les poids et biais vers la couche de sortie
void update_HO(float *errorO)
{
    for (int i=0;i<HIDDEN_SIZE;i++)
    {
        for (int j=0;j<OUTPUT_SIZE;j++)
        {
            wHO[i][j]-=LEARNING_RATE*errorO[j]*hidden[i] ;
        }
    }

    for(int i=0;i<OUTPUT_SIZE;i++)
    {
        bO[i]-=LEARNING_RATE*errorO[i];
    }
}



//met à jour les poids et biais vers la couche cachée
void update_IH(float *errorH)
{
    for (int i=0;i<INPUT_SIZE;i++)
    {
        for (int j=0;j<HIDDEN_SIZE;j++)
        {
            wIH[i][j]-=LEARNING_RATE*errorH[j]*input[i] ;
        }
    }

    for(int i=0;i<HIDDEN_SIZE;i++)
    {
        bH[i]-=LEARNING_RATE*errorH[i];
    }
}


// lance le recalcul des poids et biais en fonction du résultat voulu pour une letter
void back_propagation(int index_letter)
{
    float errorO[OUTPUT_SIZE];
    float errorH[HIDDEN_SIZE];
    calcul_errorO(errorO,index_letter);
    calcul_errorH(errorH,errorO);

    update_HO(errorO);
    update_IH(errorH);
}


//entraine le réseau avec des images données
void training(int index_letter, float **img,int len)
{
    if (len!=28) // 28 étant le nombre de pixel par ligne et colonne de la matrice image
    {
        errx(EXIT_FAILURE,"la matrice donnée ne possède pas la bonne taille");
    }
    init_weights();
    int n=0;
    for(size_t i=0;i<len;i++)
    {
        for(size_t j=0;j<len;j++)
        {
            input[n]=img[i][j];  //initialise l'input
            n++;
        }
    }
    forward();
    back_propagation(index_letter);
}



//stocke le valeurs obtenues pour les biais et les poids dans des fichiers texte
int store_res()
{
    // écrit les poids input-hidden
    FILE *fp1 = fopen("wIH.txt", "w");
    if (!fp1) 
    {
        printf("Erreur : impossible d'ouvrir le fichier wIH.txt\n");
        return -1;
    }
    for (int i=0;i<INPUT_SIZE ;i++) 
    {
        for(int j=0;j<HIDDEN_SIZE;j++)
        {
            fprintf(fp1, "%.3f|", wIH[i][j]);
        }
        fprintf(fp1,";\n");
    }

    fclose(fp1);

    // écrit les poids hidden-output
    FILE *fp2 = fopen("wHO.txt", "w");
    if (!fp2)
    {
        printf("Erreur : impossible d'ouvrir le fichier wHO.txt\n");
        return -1;
    }
    for (int i=0;i<HIDDEN_SIZE ;i++)
    {
        for(int j=0;j<OUTPUT_SIZE;j++)
        {
            fprintf(fp2, "%.3f|", wHO[i][j]);
        }
        fprintf(fp2,";\n");
    }

    fclose(fp2);


    // écrit les biais input-hidden
    FILE *fp3 = fopen("bH.txt", "w");
    if (!fp3)
    {
        printf("Erreur : impossible d'ouvrir le fichier bH.txt\n");
        return -1;
    }
    for (int i=0;i<HIDDEN_SIZE ;i++)
    {
        fprintf(fp3, "%.3f|", bH[i]);
    }

    fclose(fp3);


    // écrit les biais hidden-output
    FILE *fp4 = fopen("bO.txt", "w");
    if (!fp4)
    {
        printf("Erreur : impossible d'ouvrir le fichier bO.txt\n");
        return -1;
    }
    for (int i=0;i<OUTPUT_SIZE ;i++)
    {
        fprintf(fp4, "%.3f|", bO[i]);
    }

    fclose(fp4);

    
    return 0;
}


// lance l'entrainement avec un grand nombre d'exemple de lettres
int main()
{
    // for each exemple lancer training

    int store=store_res();
    if (store!=0) errx(EXIT_FAILURE,"erreur ecriture fichier");
    return 0;
}



