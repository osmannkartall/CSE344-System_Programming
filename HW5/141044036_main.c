#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXSTRINGSIZE 1024
#define FLORISTNUM 3
#define MAXTHREADNUM 100
#define MAXQUEUENUM 100

/* Herhangi bir isparcaciginin yapacigi isi temsil eden veri yapisi */
typedef struct task {
    void (*routine)(void *);    /* gerceklestirilecek olan isparcacigi */
    void *data;                 /* isparcaciginin parametresi          */
    struct task *next;
} task_t;

/* Isparcacigi havuzunu temsil eden veri yapisi */
typedef struct threadpool {
    pthread_t *threads;
    int thread_num;
    task_t *head;
    task_t *tail;
    int max_queue_size;
    int cur_queue_size;
    int close_queue;            /* Kuyruk'un MAXQUEUENUM ile belirlenen kapasitesi asildiginda     */
                                /* kuyruga eklemeyi durdurmak icin kullanilir.                     */
    int destroy;                /* Olusturulan isparcacigi havuzunu yok etmek icin kullanilir.     */
    int block;                  /* Kuyruk dolunca islem aliminin devam edip etmeyecigini belirler. */
    pthread_cond_t q_not_empty; /* Kuyrugun bos olmadigibi belirten kosul degiskeni                */
    pthread_cond_t q_not_full;  /* Kuyrugun tamamen dolu olmadigini belirten kosul degiskeni       */
    pthread_cond_t q_empty;     /* kuyrugun bos oldugunu belirten kosul degiskeni                  */
    pthread_mutex_t q_lock;     /* Kuyruga ulasmak icin kullanilan kilit.                          */
} *threadpool_t;

/* Cicekciye ait bilgileri tutan veri yapisi */
typedef struct florist {
    char name[MAXSTRINGSIZE];                 /* Cicekcinin ismi                             */
    double coords[2];                         /* Cicekcinin koordinatlari                    */
    char flowers[FLORISTNUM][MAXSTRINGSIZE];  /* Cicekcinin bulundurdugu ciceklerin isimleri */
    double speed;                             /* Ortalama hiz: Ornegin 1.5                   */
} florist_t;

/* Teslimat bilgilerini tutan veri yapisi */
typedef struct deliver {
    char florist_name[MAXSTRINGSIZE]; /* Teslimati yapacak olan cicekcinin ismi            */
    int which_florist;                /* Teslimati yapacak olan cicekciyi temsil eden sayi */
    char client_name[MAXSTRINGSIZE];  /* Istekte bulunan istemcinin ismi                   */
    char flower_name[MAXSTRINGSIZE];  /* Istenilen cicegin ismi                            */
    double time;                      /* Cicegin teslimati icin gecen toplam sure          */
} deliver_t;

pthread_mutex_t m;                      /* sales ve totalTime dizilerine mudahele etmek icin kullanilir. */
int sales[] = {0, 0, 0};                /* 0. indeks 1.cicekcinin yapmis oldugu toplam satis...          */
double totalTime[] = {0.0, 0.0, 0.0};   /* 0. indeks 1.cicekcinin tum teslimatlarinin toplam suresi...   */
threadpool_t first_florist;             /* Sirasiyla 1., 2. ve 3. cicekcinin isparcacigi havuzu          */
threadpool_t second_florist;
threadpool_t third_florist;

/* initialize_threadpool metodu bu metodu kullanarak isparcaciklarini olusturur. */
void execute_task(threadpool_t tp)
{
    task_t *task_p = NULL;
    int check = 0;
    while(1) {
        /* Kuyruga erismek icin kilidi al. */
        check = pthread_mutex_lock(&(tp->q_lock));
        if(check != 0){
            printf("execute_task--> pthread_mutex_lock failed\n");        
            exit(EXIT_FAILURE);
        }

        /* kuyruk bos oldugu ve isparcacigi havuzu yok edilmedigi surece */
        /* kuyruk bos degil sinyalini bekle.                             */
        while ( (tp->cur_queue_size == 0) && (!tp->destroy)) {
            check = pthread_cond_wait(&(tp->q_not_empty),&(tp->q_lock));
            if(check != 0){
                printf("execute_task--> pthread_cond_wait failed\n");        
                exit(EXIT_FAILURE);
            }
        }

        /* Isparcacigi havuzu yok edilmek isteniyorsa(destroy flag'i set edilir.) */
        /* kilidi birak ve thread'i sonlandir.                                    */
        if (tp->destroy) {
            check = pthread_mutex_unlock(&(tp->q_lock));
            if(check != 0){
                printf("execute_task--> pthread_mutex_unlock failed\n");        
                exit(EXIT_FAILURE);
            }
            return; /* pthread_exit kullanildiginda still reachable olarak 4 blok kaliyor(valgrind). */
        }

        /* Isparcacigi havuzu yok edilmedigi icin kuyrukta bekleyen istek olup olmadigina bakilir. */
        /* Kuyrugun basindaki istek alinir. */
        task_p = tp->head;
        tp->cur_queue_size --;

        /* Eger alinan istek kuyruktaki son istekse kuyruk bos olarak isaretlenir. */
        if (tp->cur_queue_size == 0)
            tp->head = tp->tail = NULL;
        /* Kuyrugun basindaki istek kuyruktan cikarilir. */
        else
            tp->head = task_p->next;
        /* Eger kuyrugun basindaki istek silinmeden once kuyruk tamamen dolu ise artik  tamamen */
        /* dolu olmadigindan kuyruga baska islerin eklenmesi icin kuyruk tamamen dolu degil     */
        /* sinyali tum bekleyen isparcaciklarina gonderilmelidir.                               */
        if ((tp->cur_queue_size == (tp->max_queue_size - 1)) && (!tp->block)){
            check = pthread_cond_broadcast(&(tp->q_not_full));
            if(check != 0){
                printf("execute_task--> pthread_cond_broadcast failed\n");        
                exit(EXIT_FAILURE);
            }
        }
        
        /* Kuyruk artik tamamen bosaldiysa bu durumda isparcacigi havuzunu ortadan kaldirmak */
        /* icin kuyruk bos sinyalini gondermemiz gerekir.                                    */
        if (tp->cur_queue_size == 0){
            check = pthread_cond_signal(&(tp->q_empty));
            if(check != 0){
                printf("execute_task--> pthread_cond_signal failed\n");        
                exit(EXIT_FAILURE);
            }
        }
        check = pthread_mutex_unlock(&(tp->q_lock));
        if(check != 0){
            printf("execute_task--> pthread_mutex_unlock failed\n");        
            exit(EXIT_FAILURE);
        }

        /* Kuyrugun basindan alinan istegi(bir isparcaciginin eklemis oldugu gorevi) gerceklestir. */
        (*(task_p->routine))(task_p->data);

        /* Is bitince ayrilan yeri birak. */
        free(task_p);
    }
}

/* task_num sayisi kadar isparcacigi olusturur.                                        */
/* q_size olusturulacak havuz icin kullanilacak kuyrugun maksimum uzunlugunu belirler. */
/* olusturulan havuz tp_p adresinde tutulur.                                           */
void initialize_threadpool(threadpool_t *tp_p, int task_num, int q_size, int block)
{
    int i = 0;
    int check = 0;
    threadpool_t tp = NULL;
    
    /* Isparcacigi havuzunu olustur. */
    tp = (threadpool_t)malloc(sizeof(struct threadpool));
    if(tp == NULL){
        printf("initialize_threadpool--> malloc(threadpool) failed\n");
        exit(EXIT_FAILURE);
    }
    
    tp->thread_num = task_num;
    tp->max_queue_size = q_size;
    tp->block = block;
 
    /* Is parcacigi havuzunun icerecegi isparcaciklari icin yer al. */
    tp->threads = (pthread_t *)malloc(sizeof(pthread_t) * task_num);    
    if (tp->threads == NULL){
        printf("initialize_threadpool--> malloc(pthread) failed\n");
        exit(EXIT_FAILURE);
    }

    tp->head = NULL;
    tp->tail = NULL;
    tp->cur_queue_size = 0;
    tp->destroy = 0;
    tp->close_queue = 0;

    /* Istek kuyruguna erisirken kullanilacak muteks ve kosul degiskenlerini al. */
    check = pthread_cond_init(&(tp->q_not_empty), NULL);
    if(check != 0){
        printf("initialize_threadpool--> pthread_cond_init failed\n");                
        exit(EXIT_FAILURE);
    }

    check = pthread_cond_init(&(tp->q_empty), NULL);
    if(check != 0){
        printf("initialize_threadpool--> pthread_cond_init failed\n");                
        exit(EXIT_FAILURE);
    }

    check = pthread_cond_init(&(tp->q_not_full), NULL);
    if(check != 0){
        printf("initialize_threadpool--> pthread_cond_init failed\n");                
        exit(EXIT_FAILURE);
    }

    check = pthread_mutex_init(&(tp->q_lock), NULL);
    if(check != 0){
        printf("initialize_threadpool--> pthread_mutex_init failed\n");        
        exit(EXIT_FAILURE);
    }

    /* Isparcaciklarini olustur. */
    for (i = 0; i < task_num; i++) {
        check = pthread_create( &(tp->threads[i]), NULL, (void *)execute_task, (void *)tp);
        if(check != 0){
            printf("initialize_threadpool--> pthread_create failed\n");                
            exit(EXIT_FAILURE);
        }
    }
    *tp_p = tp;
}

/* tp: istegin eklenecegi isparcacigi havuzu */
/* routine: eklenecek olan is.               */
/* data: eklenecek olan isin parametreleri   */
int add_task(threadpool_t tp, void (*routine)(void*), void *data)
{
    task_t *task = NULL;
    int check = 0;
    /* Kuyruga erismek icin kilidi al. */
    check = pthread_mutex_lock(&tp->q_lock);
    if(check != 0){
        printf("add_task--> pthread_mutex_lock failed\n");                
        exit(EXIT_FAILURE);
    }
    if ((tp->cur_queue_size == tp->max_queue_size) && tp->block) {
        pthread_mutex_unlock(&tp->q_lock);
        return -1;
    }
    /* Eger kuyruk tamamen doluysa kuyruk tamamen dolu degil sinyali beklenir. */
    while ((tp->cur_queue_size == tp->max_queue_size) && (!(tp->destroy || tp->close_queue))) {
        check = pthread_cond_wait(&tp->q_not_full, &tp->q_lock);
        if(check != 0){
            printf("add_task--> pthread_cond_wait failed\n");                
            exit(EXIT_FAILURE);
        }
    }

    /* Eger isparcacigi havuzu yok edildiyse alinan kilit geri birakilmalidir. */
    if (tp->destroy || tp->close_queue) {
        check = pthread_mutex_unlock(&tp->q_lock);
        if(check != 0){
            printf("add_task--> pthread_mutex_unlock failed\n");                
            exit(EXIT_FAILURE);
        }
        return -1;
    }

    /* Kuyruga bir istek eklemek icin gorev yeri alinir. */
    task = (task_t *)malloc(sizeof(task_t)); /* Yer alindi. */
    task->routine = routine; /* Yapilacak is belirlendi. */
    task->data = data; /* Yapilacak isin parametreleri atandi. */
    task->next = NULL; /* Eklenilen is kuyrugun sonuna eklenecegi icin ondan sonra gelen istek yok. */

    /* Eger kuyruk tamamen bossa eklenecek istek kuyrugun hem sonunu hem de basini gosterir. */
    if (tp->cur_queue_size == 0) {
        tp->tail = tp->head = task;
        /* Istek kuyruga eklendikten sonra herhangi bir isparcaciginin onu gerceklestirmesi icin */
        /* kuyruk bos degil sinyali gonderilir.                                                  */
        check = pthread_cond_broadcast(&tp->q_not_empty);
        if(check != 0){
            printf("add_task--> pthread_cond_broadcast failed\n");                
            exit(EXIT_FAILURE);
        }
    } 
    else {
        /* Kuyrugun en sonuna yeni istegi ekleriz. Artik kuyrugun sonu ekeldigimiz istegi gostermelidir. */
        (tp->tail)->next = task;
        tp->tail = task;
    }
    
    tp->cur_queue_size++;

    /* Kilidi birak. */
    check = pthread_mutex_unlock(&tp->q_lock);
    if(check != 0){
        printf("add_task--> pthread_mutex_unlock failed\n");                
        exit(EXIT_FAILURE);
    }
    return 0;
}

/* Olusturulmus olan tp isparcacigini yok eder. */
int destroy_threadpool(threadpool_t tp, int make_empty)
{
    int i = 0;
    int check = 0;
    task_t *temp = NULL;

    /* Kuyruga erismek icin kilidi al. */
    check = pthread_mutex_lock(&(tp->q_lock));
    if(check != 0){
        printf("destroy_threadpool--> pthread_mutex_lock failed\n");                
        exit(EXIT_FAILURE);
    }

    /* Yok etme islemi zaten yapilmaktaysa bir daha tekrarlanmamalidir. */    
    if (tp->close_queue || tp->destroy) {
        check = pthread_mutex_unlock(&(tp->q_lock));
        if(check != 0){
            printf("destroy_threadpool--> pthread_mutex_unlock failed\n");                
            exit(EXIT_FAILURE);
        }
        return -1;
    }
    
    /* Kuyruga artik istek eklenmemesi icin close_queue'ye 1 atanir. */
    tp->close_queue = 1;

    /* make_empty 1 geldigi zaman tum kuyrugun bosalmasini bekliyoruz.        */
    /* Isparcacigi havuzunu yok etmeden once kuyrukta kalan son isteklerin de */
    /* gerceklestirilmesi beklenir.                                           */
    if (make_empty == 1) {
        while (tp->cur_queue_size != 0) {
            check = pthread_cond_wait(&(tp->q_empty), &(tp->q_lock));
            if(check != 0){
                printf("destroy_threadpool--> pthread_cond_wait failed\n");                
                exit(EXIT_FAILURE);
            }
        }
    }
    
    /* destroy'a 1 atanir ki henuz isini bitirememis olan isparcaciklari destroy'un */
    /* 1 oldugunu gorunce islerini bitirsinler.                                     */
    tp->destroy = 1;
    check = pthread_mutex_unlock(&(tp->q_lock));
    if(check != 0){
        printf("destroy_threadpool--> pthread_mutex_unlock failed\n");                
        exit(EXIT_FAILURE);
    }

    /* Uykuda olan isparcaciklarinin uyanip islerini bitirmesi icin gerekli sinyalleri gonder. */
    check = pthread_cond_broadcast(&(tp->q_not_empty));
    if(check != 0){
        printf("destroy_threadpool--> pthread_cond_broadcast failed\n");                
        exit(EXIT_FAILURE);
    }
    check = pthread_cond_broadcast(&(tp->q_not_full));
    if(check != 0){
        printf("destroy_threadpool--> pthread_cond_broadcast failed\n");                
        exit(EXIT_FAILURE);
    }

    /* Havuzu yok etmeden once isparcaciklarinin islerini bitirmesini bekle. */
    for(i=0; i < tp->thread_num; i++) {
        check = pthread_join(tp->threads[i],NULL);
        if(check != 0){
            printf("destroy_threadpool--> pthread_join failed\n");                
            exit(EXIT_FAILURE);
        }
    }
    printf("All requests processed.\n");

    /* Kuyrugu kitlemek icin kullanilan muteksi yok et. Bu fonksiyonda yukarida unlock ediliyor. */
    check = pthread_mutex_destroy(&(tp->q_lock));
    if(check != 0){
        printf("destroy_threadpool--> pthread_mutex_destroy failed\n");                
        exit(EXIT_FAILURE);
    }

    /* Condition variable'ları yok et. Yukarida join ile tum thread'lerin bitmeleri bekleniyor. */
    check = pthread_cond_destroy(&(tp->q_not_full));
    if(check != 0){
        printf("destroy_threadpool--> pthread_cond_destroy failed\n");                
        exit(EXIT_FAILURE);
    }

    check = pthread_cond_destroy(&(tp->q_not_empty));
    if(check != 0){
        printf("destroy_threadpool--> pthread_cond_destroy failed\n");                
        exit(EXIT_FAILURE);
    }

    check = pthread_cond_destroy(&(tp->q_empty));
    if(check != 0){
        printf("destroy_threadpool--> pthread_cond_destroy failed\n");                
        exit(EXIT_FAILURE);
    }

    /* Is parcacigi havuzunu yok et. */
    free(tp->threads);     /* Isparcaciklari icin alinan yeri birak. */
    while(tp->head != NULL) {
        /* Kuyruk icin alinan yerleri birak. */
        temp = tp->head ->next; 
        tp->head = tp->head ->next;
        free(temp);
    }
    free(tp);
    return 0;
}

/* xf: florist x koordinati, yf: florist y koordinati */
/* xc: client x koor. yc: client y koor.              */
double find_distance(double xf, double yf, double xc, double yc){
    double result = 0.0;
    result = (double) sqrt((xf-xc)*(xf-xc) + (yf-yc)*(yf-yc));
    return result;
}

/* Her bir isparcaciginin gerceklestirecegi islemdir.                      */
/* args parametresi ile teslimat bilgilerini tutan bir veri yapisini alir. */
/* O veri yapisindaki ilgili verileri ekrana yazdirir.                     */
/* Ayrica args'in icindeki which_florist degiskenini kullanarak            */
/* ilgili saticinin satis sayisi ve toplam teslimat sureleri guncellenir.  */
void* make_deliver(void *args){
    int check = 0;
    deliver_t *request = (deliver_t *)args;
    /* Teslimat bilgilerini yazdir. */
    printf("Florist %s has delivered a %s to %s in %.0fms.\n", request->florist_name, 
                            request->flower_name, request->client_name, request->time);
    switch(request->which_florist){
        case 0 : 
                /* sales[0] ve totalTime[0] ortak degisken oldugu icin degistirmeden once kilit alinmalidir. */
                check = pthread_mutex_lock(&m);
                if(check != 0){
                    printf("destroy_threadpool--> pthread_mutex_lock failed\n");                
                    exit(EXIT_FAILURE);
                }
                sales[0]++;
                totalTime[0]+= request->time;
                /* Kilidi birak. */
                check = pthread_mutex_unlock(&m);
                if(check != 0){
                    printf("destroy_threadpool--> pthread_mutex_unlock failed\n");                
                    exit(EXIT_FAILURE);
                }
                break;
        case 1 : 
                check = pthread_mutex_lock(&m);
                if(check != 0){
                    printf("destroy_threadpool--> pthread_mutex_lock failed\n");                
                    exit(EXIT_FAILURE);
                }
                sales[1]++;
                totalTime[1]+= request->time;
                check = pthread_mutex_unlock(&m);
                if(check != 0){
                    printf("destroy_threadpool--> pthread_mutex_unlock failed\n");                
                    exit(EXIT_FAILURE);
                }
                break;
        case 2 : 
                check = pthread_mutex_lock(&m);
                if(check != 0){
                    printf("destroy_threadpool--> pthread_mutex_lock failed\n");                
                    exit(EXIT_FAILURE);
                }
                sales[2]++;
                totalTime[2]+= request->time;
                check = pthread_mutex_unlock(&m);
                if(check != 0){
                    printf("destroy_threadpool--> pthread_mutex_unlock failed\n");                
                    exit(EXIT_FAILURE);
                }
                break;
        default: printf("Error.\n");        
    }
    free(request);
}

/* Dosyadan okunan cicekcilerin bilgilerini yazdirir. */
void display_florists(florist_t florist[FLORISTNUM]){
    printf("%s->(%.0f,%.0f)->%1.1f", florist[0].name, florist[0].coords[0], florist[0].coords[1], florist[0].speed);
    printf("->%s %s %s\n", florist[0].flowers[0],florist[0].flowers[1],florist[0].flowers[2]);
    
    printf("%s->(%.0f,%.0f)->%1.1f", florist[1].name, florist[1].coords[0], florist[1].coords[1], florist[1].speed);
    printf("->%s %s %s\n", florist[1].flowers[0],florist[1].flowers[1],florist[1].flowers[2]);

    printf("%s->(%.0f,%.0f)->%1.1f", florist[2].name, florist[2].coords[0], florist[2].coords[1], florist[2].speed);
    printf("->%s %s %s\n", florist[2].flowers[0],florist[2].flowers[1],florist[2].flowers[2]);
}

int main(int argc, char *argv[])
{
    FILE * fp = NULL;
    char client_name[MAXSTRINGSIZE] = "";       /* istemcinin ismi */
    char flower_name[MAXSTRINGSIZE] = "";       /* istenilen cicek */
    double client_coords[] = {0.0, 0.0};        /* istemcinin konumu */
    char str[MAXSTRINGSIZE] = "";
    int check = 0;
    int i = 0;
    int j = 0;
    int fc = 0;                                 /* florist counter */
    char c = 'a';
    double min_dist = 0.0;                      /* en yakin cicekciye olan uzaklik */
    double florist_dist = 0.0;                  /* bulunulan cicekcinin istemciye olan uzakligi */
    florist_t florist[FLORISTNUM] = {0,0,0};    /* Cicekciler */
    int deliverer_florist = 0;                  /* teslimati yapacak olan cicekci 0: 1. cicekci(Ayse) 1: 2. cicekci... */
    int preparation_time = 0;                   /* her siparis icin rastgele olusturulan hazirlanma suresi */
    pid_t pid = 0;                              /* main surecin pid'si */
    deliver_t *deliver = NULL;                  /* teslimat bilgilerini tutan yapi */

    if(argc>2) {
        printf("Too many arguments supplied.\n");
        return -1;
    }
    else if(argc == 1){
        printf("One argument expected.(File Name)\n");
        return -1;
    }
    printf("------------------------------------------------------------------------------\n");
    printf("Florist application initializing from file: %s\n", argv[1]);
    printf("When you try files with high number of clients(more than 300)\n");
    printf("you may need to set MAXQUEUENUM and MAXTHREADNUM macros to higher numbers.\n");
    printf("If you overload any threadpool it will be blocked.           \n");
    printf("You can cancel the blocking by setting block parameter of    \n");
    printf("initialize_threadpool to zero.\n");
    printf("------------------------------------------------------------------------------\n");
    printf("The file must be same as the given data.dat. It must contain same numbers of  \n");
    printf("newlines after the last florist and the last client.                          \n"); 
    printf("------------------------------------------------------------------------------\n");
    printf("Time of delivery is computed as (distance from florist to client) * avg. speed\n");
    printf("Time of preparation is generated randomly. [10,50]\n");
    printf("Total time for a request: time of delivery + time of preparation\n");
    printf("------------------------------------------------------------------------------\n");

    /* Thread'lerin silinip silinmedigini kontrol etmek icin main calistirildiktan sonra kullanilir. */
    /* konsoldan: ps -T -p 15198 gibi..                                                              */
    pid = getpid();
    printf("main pid: %d\n", pid);

    fp = fopen(argv[1], "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    /* ----------------------------------------------------------------------------- */
    /* Cicekcileri oku. Bir tane \n okudugumuz an tum cicekcileri elde etmis oluruz. */
    /* ----------------------------------------------------------------------------- */
    for(fc = 0; fc < FLORISTNUM; fc++){
       /* Cicekcinin ismini elde et. */
        i = 0;
        while(c != ' ' && c != EOF){
            c = getc(fp);
            /* Satirin en basinda bir istemcinin isminin harfi olacak. Eger onun yerine */
            /* \n okursak tum istemciler okunmus olur. */
            if(c == '\n')
                break;
            if(c != ' ')
                florist[fc].name[i++] = c;
        }
        florist[fc].name[i] = '\0'; 

        /* Acma parantezi.. */
        getc(fp);

        /* x koordinati.. */
        i = 0;
        while(c != ',' && c != EOF){
            c = getc(fp);
            str[i++] = c;
        }
        str[i] = '\0';
        florist[fc].coords[0] = atof(str);
        
        /* y koordinati.. */
        i = 0;
        while(c != ';' && c != EOF){
            c = getc(fp);
            if(c != ' ' && c != '\n' && c != ')')
            str[i++] = c;
        }
        str[i] = '\0';
        florist[fc].coords[1] = atof(str);

        /* ' ' isareti.. */
        getc(fp);
        
        /* islem hizini oku. */
        i = 0;
        while(c != ')'){
            c = getc(fp);
            if(c != ')')
                str[i++] = c;
        }
        str[i] = '\0';
        florist[fc].speed = atof(str);

        /* ' ' ve : isareti.. */
        getc(fp);
        getc(fp);

        /* cicek ismi.. */
        i = 0;
        j = 0;
        while(c != '\n'){
            c = getc(fp);
            if(c == ','){
                florist[fc].flowers[j++][i] = '\0';
                i = 0;
            }
            else if(c != ' ' && c != '\n'){
                florist[fc].flowers[j][i++] = c;
            }
        }
        florist[fc].flowers[j][i] = '\0';
    }

    printf("%d florists have been created.\n", FLORISTNUM);

    /* block'u 1 olarak gonderdik. Boylece kuyruga eklenen istek sayisi */
    /* maksimuma ulastigi zaman ekleme yapilamaz.                       */
    /* Tam tersini istiyorsak block'u 1 yerine 0 yaparak gondeririz.    */
    initialize_threadpool(&first_florist, MAXTHREADNUM, MAXQUEUENUM,1);
    initialize_threadpool(&second_florist, MAXTHREADNUM, MAXQUEUENUM,1);
    initialize_threadpool(&third_florist, MAXTHREADNUM, MAXQUEUENUM,1);

    /* sales ve totalTime degiskenlerine erismek icin kullanilacak muteksi olustur. */
    check = pthread_mutex_init(&m, NULL);
    if(check != 0){
        printf("initialize_threadpool--> pthread_mutex_init failed\n");        
        exit(EXIT_FAILURE);
    }

    printf("Processing requests.\n");

    /* Cicekcileri okuduktan sonra bir tane \n oku. */
    getc(fp);

    /* ---------------------------------------------------------------------------- */
    /* Istemcilerin yapmis oldugu tum istekleri dosyadan oku ve sirayla isleme sok. */
    /* ---------------------------------------------------------------------------- */
    while(c != EOF){
        /* Isparcacigina gonderilecek olan parametre icin yer al. */
        deliver = (deliver_t *) malloc(sizeof(deliver_t));

        if(c == EOF)
            break;

        /* Istemcinin ismini elde et. */
        i = 0;
        while(c != ' ' && c != EOF){
            c = getc(fp);
            /* Satirin en basinda bir istemcinin isminin harfi olacak. Eger onun yerine */
            /* \n okursak tum istemciler okunmus olur. */
            if(c == '\n')
                break;
            if(c != ' ')
                deliver->client_name[i++] = c;
        }
        deliver->client_name[i] = '\0';

        /* Tum istemciler okundu devam etme. */
        if(c == '\n')
            break;

        /* Acma parantezi.. */
        getc(fp);

        /* x koordinati.. */
        i = 0;
        while(c != ',' && c != EOF){
            c = getc(fp);
            str[i++] = c;
        }
        str[i] = '\0';
        client_coords[0] = atof(str);
        
        /* y koordinati.. */
        i = 0;
        while(c != ')' && c != EOF){
            c = getc(fp);
            if(c != ' ' && c != '\n' && c != ')')
            str[i++] = c;
        }
        str[i] = '\0';
        client_coords[1] = atof(str);

        /* : isareti.. */
        getc(fp);

        /* cicek ismi.. */
        i = 0;
        while(c != '\n' && c != EOF){
            c = getc(fp);
            if(c != ' ' && c != '\n')
                deliver->flower_name[i++] = c;
        }
        deliver->flower_name[i] = '\0';

        /* ----------------------- */
        /* En yakin cicekciyi bul. */
        /* ----------------------- */
        min_dist = 999999;
        /* Karsilastirma eger bulunulan cicekcide istenilen cicek varsa yapilmalidir. */
        if(strcmp(florist[0].flowers[0], deliver->flower_name) == 0 ||
           strcmp(florist[0].flowers[1], deliver->flower_name) == 0 ||
           strcmp(florist[0].flowers[2], deliver->flower_name) == 0)
        {
            min_dist = find_distance(florist[0].coords[0], florist[0].coords[1], client_coords[0], client_coords[1]);
            deliverer_florist = 0;
        }
        for(i = 1; i < FLORISTNUM; i++){
            /* Karsilastirma eger bulunulan cicekcide istenilen cicek varsa yapilmalidir. */
            if(strcmp(florist[i].flowers[0], deliver->flower_name) == 0 ||
               strcmp(florist[i].flowers[1], deliver->flower_name) == 0 ||
               strcmp(florist[i].flowers[2], deliver->flower_name) == 0)
            {
                florist_dist = find_distance(florist[i].coords[0], florist[i].coords[1], client_coords[0], client_coords[1]);
                if(florist_dist < min_dist){
                    min_dist = florist_dist;
                    deliverer_florist = i;
                }
            }
        }
        preparation_time = (rand() % (50-10+1)) + 10;
        
        /* ----------------------- */
        /* Istegi isleme sok.      */
        /* ----------------------- */
        switch(deliverer_florist){
            case 0 : /* Teslimat icin gerekli bilgileri ayarla. */
                     strcpy(deliver->florist_name,florist[0].name);
                     deliver->time = min_dist * florist[0].speed + preparation_time;
                     deliver->which_florist = 0;
                     /* Istegi bir isparcacigina ata. */
                     add_task(first_florist, (void *)make_deliver,(void *)deliver);
                     break;
            case 1 : strcpy(deliver->florist_name,florist[1].name);
                     deliver->time = min_dist * florist[1].speed + preparation_time;
                     deliver->which_florist = 1;
                     add_task(second_florist, (void *)make_deliver,(void *)deliver);
                     break; 
            case 2 : strcpy(deliver->florist_name,florist[2].name);
                     deliver->time = min_dist * florist[2].speed + preparation_time;
                     deliver->which_florist = 2;
                     add_task(third_florist, (void *)make_deliver,(void *)deliver);                     
                     break;                 
            default: printf("Error.\n");
        }
    }

    fclose(fp);

    if(deliver != NULL)
        free(deliver);
    
    /* Teslimatlar alindiktan sonra onlarin tamamlanmasini bekleyip isparcacigi havuzunu yok et. */
    if(destroy_threadpool(first_florist, 1) == 0)
        printf("%s closing shop.\n", florist[0].name);
    if(destroy_threadpool(second_florist, 1) == 0)
        printf("%s closing shop.\n", florist[1].name);
    if(destroy_threadpool(third_florist, 1) == 0)
        printf("%s closing shop.\n", florist[2].name);

    /* Sales ve totalTimes'i kitlemek icin kullanilan muteksi yok et. En son make_deliver isparcacigi gerceklesince */
    /* unlock ediliyor. */
    check = pthread_mutex_destroy(&m);
    if(check != 0){
        printf("main--> pthread_mutex_destroy failed\n");                
        exit(EXIT_FAILURE);
    }

    printf("Sale statistics for today:\n");
    printf("-------------------------------------------------\n");
    printf("Florist         # of sales          Total time\n");
    printf("-------------------------------------------------\n");
    printf("%s              %d                  %.0fms\n", florist[0].name, sales[0], totalTime[0]);
    printf("%s             %d                  %.0fms\n", florist[1].name, sales[1], totalTime[1]);
    printf("%s             %d                  %.0fms\n", florist[2].name, sales[2], totalTime[2]);
    printf("-------------------------------------------------\n");

    return 0;
}