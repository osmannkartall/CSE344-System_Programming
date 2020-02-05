#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>

#define BUTTER 0               /* butter semaforunun numarasi                               */
#define FLOUR 1                /* flour semaforunun numarasi                                */
#define SUGAR 2                /* sugar semaforunun numarasi                                */
#define EGGS 3                 /* eggs semaforunun numarasi                                 */
#define DELIVER 4              /* DELIVER semaforunun numarasi                              */
#define READ 5                 /* READ semaforunun numarasi                                 */
#define WRT 6                  /* WRT semaforunun numarasi                                  */
#define KEYPATH "/dev/null"    /* shared memory ve semafor icin kullanilan key'in path'i    */
#define KEYID 'a'              /* shared memory ve semafor icin kullanilan key'in id degeri */
#define SHMSIZE sizeof(int)*2  /* shared memory'nin boyutu: 2'lik int array'i               */
#define PROCESSCOUNT 7         /* Malzemeler 2'li olarak kullaniliyor. 4'un 2'li            */
                               /* kombinasyonundan toplam 6 farkli process'i chef olarak    */
                               /* temsil edebiliriz. 1 tane de wholesaler process'i vardir. */

struct sembuf ops[2];
int check = 0, semid = 0, shmid = 0;
int *shmaddress = NULL;
key_t semkey = 0, shmkey = 0;    
pid_t pidArray[] = {0,0,0,0,0,0,0,0};

void termination_handler(int sig){
    if(sig == SIGINT){
        if(pidArray[0] == getpid())
            printf("---->You abort the program execution!!!!!!\n");
        while(wait(NULL)!=-1);    
        
        /* Semafor setini sil. main surecin silmesi yeterlidir. */    
        if(pidArray[0] == getpid()){
            check = semctl( semid, 1, IPC_RMID );
            if (check ==-1){
                printf("termination_handler--> semctl failed\n");
                exit(EXIT_FAILURE);
            }
        }
        
        /* Herhangi bir process paylasimli hafizanin shmadress ile        */
        /* baglantisini kaldirmadiysa ilk gelen detaching islemini yapar. */    
        if((shmaddress = shmat(shmid, NULL, 0)) != NULL){
            check = shmdt(shmaddress);
            if (check ==-1){
                printf("termination_handler--> shmdt failed\n");
                exit(EXIT_FAILURE);
            }
        }    

        check = shmctl(shmid, IPC_RMID, NULL);
        if (check ==-1){
            printf("termination_handler--> shmctl failed\n");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }    
}

/* Verilen int degerine karsilik gelen malzeme ismini name'e kopyalar. */
void setIngreName(char *name, int ingreNo){
    switch(ingreNo){
        case 0: strcpy(name,"butter"); break;
        case 1: strcpy(name,"flour"); break;
        case 2: strcpy(name,"sugar"); break;
        case 3: strcpy(name,"eggs"); break;
    }
}

/* first ve second parametresi ile gelen birbirinden farkli numarali iki semaforun  */
/* atomik olarak wait islemini gerceklestirir.                                      */
int getIngredients(int first, int second){  
    ops[0].sem_num = first;
    ops[1].sem_num = second;
    ops[0].sem_op = ops[1].sem_op = -1;
    ops[0].sem_flg = ops[1].sem_flg = 0; 
    return semop(semid, ops, 2);
}

/* first ve second parametresi ile gelen birbirinden farkli numarali iki semaforun  */
/* atomik olarak post islemini gerceklestirir.                                      */
int deliverIngredients(int first, int second){
    ops[0].sem_num = first;
    ops[1].sem_num = second;
    ops[0].sem_op = ops[1].sem_op = 1;
    ops[0].sem_flg = ops[1].sem_flg = IPC_NOWAIT; 
    return semop(semid, ops, 2);
}

/* semNum parametresine denk gelen semaforun wait islemini yapar. */
int waitV(int semNum){
    ops[0].sem_num = semNum;
    ops[0].sem_op = -1;
    ops[0].sem_flg = 0; 
    return semop(semid, ops,1);
}

/* semNum parametresine denk gelen semaforun post islemini yapar. */ 
int postV(int semNum){
    ops[0].sem_num = semNum;
    ops[0].sem_op = 1;
    ops[0].sem_flg = IPC_NOWAIT; 
    return semop(semid, ops,1);
}

int main()
{
    int randIngredient1 = 0;
    int randIngredient2 = 0;
    int i = 0;
    char ingre1name[7] = "";
    char ingre2name[7] = "";
    struct sigaction signal;
    short arr[] = {0,0,0,0,0,0,1};
    /* BUTTER, FLOUR, SUGAR, EGGS, DELIVER, READ, WRT semaforlari icin baslangic degerleri...   */
    /* Ilk basta sadece WRT 1 olmali ki wholesaler paylasimli hafizaya rastgele iki malzeme     */
    /* yazsin ve chef'ler de READ 0 oldugu icin o malzemenin yazilmasini beklesin.              */

    pidArray[0] = getpid();

    /* CTRL+C sonlandirmasi icin SIGINT'in handler'ini ayarla. */
    memset (&signal, 0, sizeof (signal));
    signal.sa_handler = termination_handler;
    sigemptyset(&signal.sa_mask);
    sigaddset(&signal.sa_mask, SIGINT);
    if (sigaction(SIGINT, &signal, NULL) == -1) {
        fprintf(stderr, "Cannot handle..");
        exit(EXIT_FAILURE);
    }

    /* Kullanilacak semafor kumesi icin IPC key'i olustur. */
    if ((semkey = ftok(KEYPATH, KEYID)) == (key_t)-1) {
        perror("ftok: semaphore-> in main\n"); 
        exit(EXIT_FAILURE);
    }

    /* Kullanilacak paylasimli hafiza icin IPC key'i olustur. */
    if ((shmkey = ftok(KEYPATH, KEYID)) == (key_t)-1) {
        perror("ftok: shared memory-> in main\n"); 
        exit(EXIT_FAILURE);
    }

    /* Semafor kumesi icin verilen key'i kullanarak */
    /* semafor kumesi segmentini olustur.           */
    if((semid = semget( semkey, 7, 0666 | IPC_CREAT)) == -1){
        perror("semget-> in main\n"); 
        exit(EXIT_FAILURE);
    } 
    
    /* Semafor kumesindeki semaforlarin degerlerini arr array'i ile degistir. */
    check = semctl(semid, 1, SETALL, arr);
    if(check == -1){
        perror("semctl-> in main"); 
        exit(EXIT_FAILURE);
    }

    /* Paylasimli hafiza icin verilen key'i kullanarak */
    /* paylasimli hafiza segmentini olustur.           */
    shmid = shmget(shmkey, SHMSIZE, 0666 | IPC_CREAT | IPC_EXCL);
    if(shmid == -1){
        perror("shmget-> in main\n");
        exit(EXIT_FAILURE);
    }

    /* Paylasimli hafiza segment'ini shmaddress'e bagla. */
    shmaddress = shmat(shmid, NULL, 0);
    if(shmaddress == NULL){
        perror("shmat-> in main\n");
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < PROCESSCOUNT; i++)
    {
        if(fork() == 0)
        {
            switch(i){
                case 0:
                    pidArray[i+1] = getpid();
                    while(1){ 
                        printf("Chef1 is waiting for butter and sugar.\n");

                        /* Paylasimli hafizadan okumak icin istekte bulun. */                        
                        waitV(READ);

                        shmid = shmget(shmkey, SHMSIZE, 0666);
                        if(shmid == -1){
                            perror("shmget-> in main/CH1\n");
                            exit(EXIT_FAILURE);
                        }

                        /* Paylasimli hafiza segment'ini shmaddress'e bagla. */
                        /* Sadece okuma yapmak icin baglanilir. */
                        shmaddress = (int *)shmat(shmid, 0, SHM_RDONLY);
                        if(shmaddress == NULL){
                            perror("shmat-> in main/CH1\n");
                            exit(EXIT_FAILURE);
                        }

                        /* Paylasimli hafizadan alinan malzeme bilgilerini sakla. */
                        randIngredient1 = shmaddress[0]; 
                        randIngredient2 = shmaddress[1];

                        /* Paylasimli hafiza segment'inin shmaddress ile baglantisini kaldir. */
                        shmdt((void *) shmaddress);

                        if((randIngredient1 == BUTTER && randIngredient2 == SUGAR)
                                || (randIngredient2 == BUTTER && randIngredient1 == SUGAR)){
                            
                            /* Paylasimli hafizadan okunan malzemeler bu chef'in aradigi malzemeler */
                            /* oldugu icin bu malzemeleri temsil eden semaforlar icin wait yapilir. */
                            check = getIngredients(BUTTER,SUGAR);
                            if (check == -1){
                                perror("semop-> in main/CH1\n");
                                exit(EXIT_FAILURE);
                            }
                            printf("Chef1 has taken the butter\n");
                            printf("Chef1 has taken the sugar\n");
                            printf("Chef1 is preparing the dessert.\n");
                            printf("Chef1 has delivered the dessert to the wholesaler.\n");

                            /* Sekerpare olusturuldugu icin DELIVER semaforu post edilir. */
                            check = postV(DELIVER);
                            if (check == -1){
                                perror("semop-> in main/CH1\n");
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                            /* Paylasimli hafizadan okunan malzemelerin en az birisi bu chef'in aradigi */
                            /* malzemelerden olmadigi icin bu chef sekerpareyi hazirlayamaz.            */
                            /* Dolayisi ile READ semaforunu post etmeli ki bu malzemelere asil ihtiyaci */
                            /* olan sefin onlari okuyabilme sansi olsun.                                */
                            postV(READ);    
                    }
                    break;
                case 1: /* flour and butter */
                    pidArray[i+1] = getpid(); 
                    while(1){ 
                        printf("Chef2 is waiting for flour and butter.\n");
                        
                        waitV(READ);

                        shmid = shmget(shmkey, SHMSIZE, 0666);
                        if(shmid == -1){
                            perror("shmget-> in main/CH2\n");
                            exit(EXIT_FAILURE);
                        }

                        shmaddress = (int *)shmat(shmid, 0, SHM_RDONLY);
                        if(shmaddress == NULL){
                            perror("shmat-> in main/CH2\n");
                            exit(EXIT_FAILURE);
                        }

                        randIngredient1 = shmaddress[0]; 
                        randIngredient2 = shmaddress[1];

                        shmdt((void *) shmaddress);

                        if((randIngredient1 == FLOUR && randIngredient2 == BUTTER)
                                || (randIngredient2 == FLOUR && randIngredient1 == BUTTER)){ 
                            check = getIngredients(FLOUR,BUTTER);
                            if (check == -1){
                                perror("semop-> in main/CH2\n");
                                exit(EXIT_FAILURE);
                            }
                            printf("Chef2 has taken the flour\n");
                            printf("Chef2 has taken the butter\n");
                            printf("Chef2 is preparing the dessert.\n");
                            printf("Chef2 has delivered the dessert to the wholesaler.\n");
                            check = postV(DELIVER);
                            if (check == -1){
                                perror("semop-> in main/CH2\n");
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                            postV(READ);    
                    }
                    break;
                case 2: /* eggs and flour */
                    pidArray[i+1] = getpid(); 
                    while(1){ 
                        printf("Chef3 is waiting for eggs and flour.\n");
                        
                        waitV(READ);

                        shmid = shmget(shmkey, SHMSIZE, 0666);
                        if(shmid == -1){
                            perror("shmget-> in main/CH3\n");
                            exit(EXIT_FAILURE);
                        }

                        shmaddress = (int *)shmat(shmid, 0, SHM_RDONLY);
                        if(shmaddress == NULL){
                            perror("shmat-> in main/CH3\n");
                            exit(EXIT_FAILURE);
                        }

                        randIngredient1 = shmaddress[0]; 
                        randIngredient2 = shmaddress[1];

                        shmdt((void *) shmaddress);

                        if((randIngredient1 == EGGS && randIngredient2 == FLOUR)
                                || (randIngredient2 == EGGS && randIngredient1 == FLOUR)){ 
                            check = getIngredients(EGGS,FLOUR);
                            if (check == -1){
                                perror("semop-> in main/CH3\n");
                                exit(EXIT_FAILURE);
                            }
                            printf("Chef3 has taken the eggs\n");
                            printf("Chef3 has taken the flour\n");
                            printf("Chef3 is preparing the dessert.\n");
                            printf("Chef3 has delivered the dessert to the wholesaler.\n");
                            check = postV(DELIVER);
                            if (check == -1){
                                perror("semop-> in main/CH3\n");
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                            postV(READ);    
                    }
                    break;
                case 3: /* butter and eggs */
                    pidArray[i+1] = getpid(); 
                    while(1){
                        printf("Chef4 is waiting for butter and eggs.\n");
                        
                        waitV(READ);

                        shmid = shmget(shmkey, SHMSIZE, 0666);
                        if(shmid == -1){
                            perror("shmget-> in main/CH4\n");
                            exit(EXIT_FAILURE);
                        }

                        shmaddress = (int *)shmat(shmid, 0, SHM_RDONLY);
                        if(shmaddress == NULL){
                            perror("shmat-> in main/CH4\n");
                            exit(EXIT_FAILURE);
                        }

                        randIngredient1 = shmaddress[0]; 
                        randIngredient2 = shmaddress[1];

                        shmdt((void *) shmaddress);

                        if((randIngredient1 == BUTTER && randIngredient2 == EGGS)
                                || (randIngredient2 == BUTTER && randIngredient1 == EGGS)){ 
                            check = getIngredients(BUTTER,EGGS);
                            if (check == -1){
                                perror("semop-> in main/CH4\n");
                                exit(EXIT_FAILURE);
                            }
                            printf("Chef4 has taken the butter\n");
                            printf("Chef4 has taken the eggs\n");
                            printf("Chef4 is preparing the dessert.\n");
                            printf("Chef4 has delivered the dessert to the wholesaler.\n");
                            check = postV(DELIVER);
                            if (check == -1){
                                perror("semop-> in main/CH4\n");
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                            postV(READ);        
                    }
                    break;
                case 4: /* eggs and sugar */
                    pidArray[i+1] = getpid(); 
                    while(1){ 
                        printf("Chef5 is waiting for eggs and sugar.\n");
                        
                        waitV(READ);

                        shmid = shmget(shmkey, SHMSIZE, 0666);
                        if(shmid == -1){
                            perror("shmget-> in main/CH5\n");
                            exit(EXIT_FAILURE);
                        }

                        shmaddress = (int *)shmat(shmid, 0, SHM_RDONLY);
                        if(shmaddress == NULL){
                            perror("shmat-> in main/CH5\n");
                            exit(EXIT_FAILURE);
                        }

                        randIngredient1 = shmaddress[0]; 
                        randIngredient2 = shmaddress[1];

                        shmdt((void *) shmaddress);

                        if((randIngredient1 == EGGS && randIngredient2 == SUGAR)
                                || (randIngredient2 == EGGS && randIngredient1 == SUGAR)){    
                            check = getIngredients(EGGS,SUGAR);
                            if (check == -1){
                                perror("semop-> in main/CH5\n");
                                exit(EXIT_FAILURE);
                            }
                            printf("Chef5 has taken the eggs\n");
                            printf("Chef5 has taken the sugar\n");
                            printf("Chef5 is preparing the dessert.\n");
                            printf("Chef5 has delivered the dessert to the wholesaler.\n");
                            check = postV(DELIVER);
                            if (check == -1){
                                perror("semop-> in main/CH5\n");
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                            postV(READ);                                
                    }
                    break;
                case 5: /* sugar and flour */
                    pidArray[i+1] = getpid(); 
                    while(1){ 
                        printf("Chef6 is waiting for sugar and flour.\n");

                        waitV(READ);

                        shmid = shmget(shmkey, SHMSIZE, 0666);
                        if(shmid == -1){
                            perror("shmget-> in main/CH6\n");
                            exit(EXIT_FAILURE);
                        }

                        shmaddress = (int *)shmat(shmid, 0, SHM_RDONLY);
                        if(shmaddress == NULL){
                            perror("shmat-> in main/CH6\n");
                            exit(EXIT_FAILURE);
                        }

                        randIngredient1 = shmaddress[0]; 
                        randIngredient2 = shmaddress[1];

                        shmdt((void *) shmaddress);

                        if((randIngredient1 == SUGAR && randIngredient2 == FLOUR)
                                || (randIngredient2 == SUGAR && randIngredient1 == FLOUR)){
                        
                            check = getIngredients(SUGAR,FLOUR);
                            if (check == -1){
                                perror("semop-> in main/CH6\n");
                                exit(EXIT_FAILURE);
                            }
                            printf("Chef6 has taken the sugar\n");
                            printf("Chef6 has taken the flour\n");
                            printf("Chef6 is preparing the dessert.\n");
                            printf("Chef6 has delivered the dessert to the wholesaler.\n");
                            check = postV(DELIVER);
                            if (check == -1){
                                perror("semop-> in main/CH6\n");
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                            postV(READ);
                    }
                    break;
                case 6:
                    pidArray[i+1] = getpid();
                    while(1){
                        /* Rastgele iki malzeme sec. Ikisi de birbirinden farkli olsun. */
                        randIngredient1 = rand() % 4;
                        randIngredient2 = randIngredient1;
                        while(randIngredient1 == randIngredient2)
                            randIngredient2 = rand() % 4;
                        
                        /* Paylasimli hafizaya yazmak icin istekte bulun. */
                        waitV(WRT);

                        shmid = shmget(shmkey, SHMSIZE, 0666);
                        if(shmid == -1){
                            perror("shmget-> in main/CH7\n");
                            exit(EXIT_FAILURE);
                        }

                        /* Paylasimli hafiza segment'ini shmaddress'e bagla. */
                        /* Yazma yapmak icin baglanilir. */
                        shmaddress = (int *)shmat(shmid, 0, 0);
                        if(shmaddress == NULL){
                            perror("shmat-> in main/CH7\n");
                            exit(EXIT_FAILURE);
                        }

                        /* ingredient'lari shared memory alanina ekle. */
                        shmaddress[0] = randIngredient1;
                        shmaddress[1] = randIngredient2;

                        /* Paylasimli hafiza segment'inin shmaddress ile baglantisini kaldir. */
                        shmdt((void *) shmaddress);

                        setIngreName(ingre1name, randIngredient1);
                        setIngreName(ingre2name, randIngredient2);
                        printf("Wholesaler delivers %s and %s\n", ingre1name, ingre2name);
                        
                        /* Wholesaler'a sekerpare gelince(DELIVER post edilince) tekrardan yazma islemi */
                        /* yapabilsin diye WRT post edilir.                                             */
                        postV(WRT);

                        /* Yazma islemi tamamlandiktan sonra chef surecler, paylasilmis hafizaya wholesaler */
                        /* tarafindan yazilmis olan malzemeleri alabilsinler diye READ'i post et.           */
                        postV(READ);

                        /* Rastgele secilen iki malzemeyi post et. */
                        check = deliverIngredients(randIngredient1, randIngredient2);
                        if (check == -1){
                            perror("semop-> in main/CH7\n");
                            exit(EXIT_FAILURE);
                        }
                        printf("Wholesaler is waiting for the dessert\n");

                        /* Wholesaler verdigi malzemeleri alan chef'in sekerpareyi hazirlamasini bekliyor. */
                        /* Bu sayede, wholesaler ilgili chef sekerpareyi hazirlayana kadar paylasimli      */
                        /* hafizaya tekrar yazamadan burada bloke olacak.                                  */
                        check = waitV(DELIVER);
                        if (check == -1){
                            perror("semop-> in main/CH7\n");
                            exit(EXIT_FAILURE);
                        }
                        printf("Wholesaler has obtained the dessert and left to sell it\n");
                    }
                    break;
            }
        }
    }
    for(i = 0; i < PROCESSCOUNT; i++)
        wait(NULL);

    return 0; 
}