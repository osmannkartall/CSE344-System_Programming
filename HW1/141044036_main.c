/* ------------------------------------ */
/* Osman Kartal tarafindan olusturuldu. */
/*		ogrenci no: 141044036			*/
/*					HW 01				*/
/* ------------------------------------ */

/* ------------------------------------ */
/* 				INCLUDE'LAR 			*/
/* ------------------------------------ */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

/* -------------------------------------------------------- */
/* 						FONKSIYONLAR						*/
/* -------------------------------------------------------- */
/* getValueOrOffsetLong()									*/
/*															*/
/* long tipindeki degerleri buffer'dan cikarip return eder. */
/* 		ornegin; width, length, strip offset 				*/
/*		degerlerinden biri veya dosya icindeki				*/
/*		belli offset degerleri 								*/
/* -------------------------------------------------------- */
long getValueOrOffsetLong(char buffer[], int index, char byteOrder)
{
    long data = 0; /* value/offset */
    long n = 0;
    
    int i = 0, j = 0;
    
    /* buffer'da tutulan degerleri byte byte cikar ve basamak ve sayi */
    /* degerlerine gore gerekli value veya offset'i hesapla.          */
    if(byteOrder == 'M'){
        
        for(i = 0; i < 4; ++i){
            n = buffer[index+3-i];
    
            while(n != 0){
                data += (n % 10) * pow(10,j);
                n /= 10;
                ++j;
            }
        }
    }
    
    else if(byteOrder == 'I'){
    
        for(i = 0; i < 4; ++i){
            n = buffer[index+i];
   
            while(n != 0){
                data += (n % 10) * pow(10,j);
                n /= 10;
                ++j;
            }
        }
    }
 
    return data;
}

/* getValueOrOffsetShort()									 */
/*															 */
/* short tipindeki degerleri buffer'dan cikarip return eder. */
/* 		ornegin; image width, image length,  				 */
/*		strip offset, IFD'deki entry sayisi, 				 */
/*		color space gibi degerleri  						 */
/* --------------------------------------------------------- */
short getValueOrOffsetShort(char buffer[], int index, char byteOrder)
{
    short data = 0; /* value/offset */
    short n = 0;
    
    int i = 0, j = 0;

    /* buffer'da tutulan degerleri byte byte cikar ve basamak ve sayi */
    /* degerlerine gore gerekli value veya offset'i hesapla.          */
    if(byteOrder == 'M'){
        
        for(i = 0; i < 2; ++i){
            n = buffer[index+1-i];

            while(n != 0){
            data += (n % 10) * pow(10,j);
            n /= 10;
            ++j;
            }
        }
    }
    else if(byteOrder == 'I'){
        
        for(i = 0; i < 2; ++i){
            n = buffer[index+i];

            while(n != 0){
                data += (n % 10) * pow(10,j);
                n /= 10;
                ++j;
            }
        }
    } 
    
    return data;
}

/* getValueOrOffsetUnShort()										  */
/*																	  */
/* unsigned short tipindeki degerleri buffer'dan cikarip return eder. */
/* 		ornegin; image width, image length,  						  */
/*		strip offset, IFD'deki entry sayisi, 						  */
/*		color space gibi degerler 	  								  */
/* ------------------------------------------------------------------ */
unsigned short getValueOrOffsetUnShort(char buffer[], int index, char byteOrder)
{
    unsigned short data = 0; /* value/offset */
    
    unsigned short n = 0;
    
    int i = 0, j = 0;

    /* buffer'da tutulan degerleri byte byte cikar ve basamak ve sayi */
    /* degerlerine gore gerekli value veya offset'i hesapla.          */
    if(byteOrder == 'M'){
        
        for(i = 0; i < 2; ++i){
            n = buffer[index+3-i];
                        
            while(n != 0){
                data += (n % 10) * pow(10,j);
                n /= 10;
                ++j;
            }
        }
    }
    else if(byteOrder == 'I'){
        
        for(i = 0; i < 2; ++i){
            n = buffer[index+i];
            
            while(n != 0){
                data += (n % 10) * pow(10,j);
                n /= 10;
                ++j;
            }
        }
    } 

    return data;
}

/* getTags() 													*/
/*																*/
/* TIFF dosyasindaki entry'leri buffer'dan cikarip return eder. */
/*																*/
/* not: Dosyadan tag'leri okudugumuzda ister decimal yazdirmaya */
/* 		ister hex. yazdirmaya calisalim ikisinde de hex 		*/
/* 		degerini veriyor. Bu yüzden getValueOrOffsetShort tam   */  
/*		olarak kullanilamadigindan getTags fonksiyonu yazildi.	*/
/* ------------------------------------------------------------ */
short getTags(char buffer[], int index, char byteOrder)
{
    short tagValue = 0;
    short n = 0;

    int j = 0;

    if(byteOrder == 'M'){
        
        /* Okunan 2 byte'lik sayinin son byte'i 0 ise 2 basamak degeri arttirmamiz lazim.  */
        /* Bunu da j'yi 2 arttirarak saglariz.                                             */
        if(buffer[index+1] == '\0')
            j+=2;
        
        /* Okunan 2 byte'lik sayinin son byte'i 0'dan farkli ise bu byte'tan sayi ve */
        /* basamak degerleri elde edilir.                                            */
        else{
            n = buffer[index+1];
            
            /* buffer'da hex. degerleri oldugu icin 16'ya bolerek her bir basamagin sayi degerleri */
            /* elde edilir. Bunlar da uygun olarak 16'nin kuvvetleri ile carpilarak buffer'daki    */
            /* sayinin decimal karsiligi bulunur.                                                  */        
            while(n != 0){
                tagValue += (n % 16) * pow(16,j);
                n /= 16;
                /* Sayiyi 16'ya boldukten sonra j'yi(basamak degerini) bir arttir. */ 
                ++j;
            }
            
            /* Okunan 2 byte'lik sayinin son byte'i 16'dan kucukse bir basamak daha arttir. */
            /* Ornegin toplam 2 byte'lik sayi 0105 olsun. Bu durumda 05'teki 0 icin de      */
            /* basamak degeri arttirilmali. O da, dongude degil burada arttirilir.          */
            if(tagValue < 16)
                ++j;
        
        }
        
        /* Most significant byte'i decimal'a cevir.  */
        n = buffer[index];
        
        while(n != 0){
            tagValue += (n % 16) * pow(16,j);
            n /= 16;
            /* Sayiyi 16'ya boldukten sonra j'yi(basamak degerini) bir arttir. */ 
            ++j;
        }
    
    }
    else if(byteOrder == 'I'){
        
        /* Okunan 2 byte'lik sayinin ilk byte'i 0 ise 2 basamak degeri arttirmamiz lazim.  */
        /* Bunu da j'yi 2 arttirarak saglariz.                                             */
        if(buffer[index] == '\0')
            j+=2;
        
        /* Okunan 2 byte'lik sayinin ilk byte'i 0'dan farkli ise bu byte'tan sayi ve */
        /* basamak degerleri elde edilir.                                             */
        else{
            n = buffer[index];
            
            /* buffer'da hex. degerleri oldugu icin 16'ya bolerek her bir basamagin sayi degerleri */
            /* elde edilir. Bunlar da uygun olarak 16'nin kuvvetleri ile carpilarak buffer'daki    */
            /* sayinin decimal karsiligi bulunur.                                                  */        
            while(n != 0){
                tagValue += (n % 16) * pow(16,j);
                n /= 16;
                /* Sayiyi 16'ya boldukten sonra j'yi(basamak degerini) bir arttir. */ 
                ++j;
            }
            
            /* Okunan 2 byte'lik sayinin son byte'i 16'dan kucukse bir basamak daha arttir. */
            /* Ornegin toplam 2 byte'lik sayi 0105 olsun. Bu durumda 05'teki 0 icin de      */
            /* basamak degeri arttirilmali. O da, dongude degil burada arttirilir.          */
            if(tagValue < 16)
                ++j;
        
        }
        
        /* Least significant byte'i decimal'a cevir.  */
        n = buffer[index+1];
        
            while(n != 0){
                tagValue += (n % 16) * pow(16,j);
                n /= 16;
                /* Sayiyi 16'ya boldukten sonra j'yi(basamak degerini) bir arttir. */ 
                ++j;
            }
    
    } 
    
    return tagValue;
}

/* getHeaderPart() 																	*/
/*																					*/
/* Verilen tif uzantili dosyanin byte order, image length, image width strip offset */
/* bits per sample ve color space degerlerini elde eder. 							*/
/* -------------------------------------------------------------------------------- */
void getHeaderPart(char fileName[], char *byteOrder, long *length, long *width, long *stripOffset,
                   short *bitsPerSample, short *colorSpace)
{
    FILE *fp;

    char buffer[12] = ""; 
    /* En buyuk okuma bir adet IFD entry icin yapilir. O da 12 byte'tir. */
    /* Bu yuzden 12 byte'lik buffer olusturmamiz yeterlidir.             */

    int i = 0, jc = 0;

    long offset = 0;
        
    short entryNum = 0, tagType = 0, fieldType = 0, count = 0;
   
    fp = fopen(fileName, "rb");
    if(fp != NULL){

        /* Header okunur. buffer'a 8 byte okuma yapilir. */      
        fread(buffer, 1, 8, fp);

        /* Header'in ilk 2 byte'i byte-order bilgisini verir.          */
        /* 4D4D.H ise motorola, 4949.H ise intel byte dizilimi vardir. */
        if(buffer[0] == 0x4D && buffer[1] == 0x4D)
            *byteOrder = 'M';
        else if(buffer[0] == 0x49 && buffer[1] == 0x49)
            *byteOrder = 'I';
        else if(buffer[0] != 0x49 && buffer[0] != 0x4D){
            fprintf(stderr, "Invalid byte-order type.\n");
            exit(-1);            
        }

        /* Header'in kalan son 4 byte'i 0.IFD'ye olan offset'i verir.  */
        /* index'i 4 olarak vermeliyiz ki son 4 byte'a ulasabilelim. */
        offset = getValueOrOffsetLong(buffer, 4, *byteOrder);
        
        /* TIFF'in sadece ilk byte'inin offset'i kesinlikle 0 olacak ve */
        /* bir ustteki okumayla offset zaten 0'dan farkli olacagi icin  */
        /* dongu calisacaktir.                                          */
        /* TIFF'te baska IFD kalmayinca offset 0 okunacaktir.           */
        while(offset != 0){
            /* offset okunduktan sonra IFD'nin baslangicina gidilir. */  
            fseek(fp, offset, SEEK_SET);

            /* IFD'nin baslangicina gidildikten sonra IFD'nin icinde */
            /* kac tane entry oldugunu ogrenmek icin 2 byte okunur.  */
            fread(buffer, 1, 2, fp);
      
            entryNum = getValueOrOffsetShort(buffer, 0, *byteOrder);

            /* entryNum kadar dongu doneriz ki tum tag'lere ve icindeki */    
            /* bilgilere ulasabilelim.                                  */
            for(i=0; i<entryNum; i++){
       
                /* Her IFD entry'si 12 byte'tan olusur.       */
                /* 12 byte'lik okuma yapilir.                 */    
                /* Value tag tipi ve count'a gore belirlenir. */
                fread(buffer, 1, 12, fp);
       
                /* 0-1. byte'lardan tag bilgisi alinir. */
                tagType = getTags(buffer, 0, *byteOrder);
                
                switch(tagType){
                    /* tagType 256 decimal degerindeyse bize goruntunun piksel */
                    /* sayisi olarak genisligini verir.                        */
                    case 256: 
                        /* 2-3. byte'lardan field type bilgisi alinir. */
                        fieldType = getValueOrOffsetUnShort(buffer, 2, *byteOrder);
                        
                        /* 4-7. byte'lardan count bilgisi alinir. */
                        count = getValueOrOffsetLong(buffer, 4, *byteOrder);

                        /* 8-11. byte'lardan field'in degeri okunur. */
                        /* image width degeri short ise... */
                        for(jc = 0; jc < count; ++jc){                      
                            if(fieldType == 3)
                                *width = getValueOrOffsetShort(buffer, 8, *byteOrder);
                            /* image width degeri long ise... */                        
                            else
                                *width = getValueOrOffsetLong(buffer, 8, *byteOrder);
                        }
                        break;

                    /* tagType 257 decimal degerindeyse bize goruntunun piksel */
                    /* sayisi olarak uzunlugunu verir.                         */
                    case 257: 
                        /* 2-3. byte'lardan field type bilgisi alinir. */
                        fieldType = getValueOrOffsetUnShort(buffer, 2, *byteOrder);

                        /* 4-7. byte'lardan count bilgisi alinir. */
                        count = getValueOrOffsetLong(buffer, 4, *byteOrder);

                        /* 8-11. byte'lardan field'in degeri okunur. */
                        /* image length degeri short ise... */
                        for(jc = 0; jc < count; ++jc){                                                
                            if(fieldType == 3)
                                *length  =  getValueOrOffsetShort(buffer, 8, *byteOrder);
                            /* image length degeri long ise... */                        
                            else
                                *length = getValueOrOffsetLong(buffer, 8, *byteOrder);
                        }
                        break;

                    /* tagType 258 decimal degerindeyse bize goruntunun */
                    /* sample basina dusen bit sayisini                 */
                    case 258: 
                        /* 2-3. byte'lardan field type bilgisi alinir. */                        
                        fieldType = getValueOrOffsetUnShort(buffer, 2, *byteOrder);

                        /* 4-7. byte'lardan count bilgisi alinir. */
                        count = getValueOrOffsetLong(buffer, 4, *byteOrder);

                        /* 8-11. byte'lardan field'in degeri okunur. */
                        /* bitsPerSample degeri sadece short type'tir. */
                        for(jc = 0; jc < count; ++jc)                                                 
                            *bitsPerSample = getValueOrOffsetShort(buffer, 8, *byteOrder);
    
                        break;

                    /* tagType 262 decimal degerindeyse bize goruntunun */
                    /* color space degerini verir.                      */
                    case 262:
                        /* 2-3. byte'lardan field type bilgisi alinir. */
                        fieldType = getValueOrOffsetUnShort(buffer, 2, *byteOrder);

                        /* 4-7. byte'lardan count bilgisi alinir. */
                        count = getValueOrOffsetLong(buffer, 4, *byteOrder);
 
                        /* 8-11. byte'lardan field'in degeri okunur. */
                        /* colorSpace degeri sadece short type'tir. */
                        for(jc = 0; jc < count; ++jc)                                                                           
                            *colorSpace = getValueOrOffsetShort(buffer, 8, *byteOrder);
                        break;

                    /* tagType 273 decimal degerindeyse bize goruntunun    */
                    /* her strip(serit)'i icin byte offset degerini verir. */
                    case 273: 
                        /* 2-3. byte'lardan field type bilgisi alinir. */
                        fieldType = getValueOrOffsetUnShort(buffer, 2, *byteOrder);
                        
                        /* 4-7. byte'lardan count bilgisi alinir. */
                        count = getValueOrOffsetLong(buffer, 4, *byteOrder);
            
                        /* 8-11. byte'lardan field'in degeri okunur. */
                        /* stripOffset degeri short ise... */
                        for(jc = 0; jc < count; ++jc){                                                                           
                            if(fieldType == 3)
                                *stripOffset = getValueOrOffsetShort(buffer, 8, *byteOrder);
                            /* stripOffset degeri long ise... */
                            else
                                *stripOffset = getValueOrOffsetLong(buffer, 8, *byteOrder);
                        }
                        break;

                    default: /* printf("%d.D wasn't being processed!\n", tagType); */
                        break;
                }
            }

            /* Bir IFD'nin entry'leri okunduktan sonra 4 byte okuma yapilir. */
            /* Bu 4 byte offset degeridir ve varsa bir sonraki IFD'ye olan   */
            /* mesafeyi belirler. Eger bu offset degeri 0 ise dosyada        */
            /* baska bir IFD kalmadi demektir.                               */
            fread(buffer, 1, 4, fp);
            offset = getValueOrOffsetLong(buffer, 0, *byteOrder);
        }

        fclose(fp);
    }
    else{
        printf("The file could not be opened.\n");
        printf("Errno: %d\n", errno);
    }
}

/* displayPixels() 										   */
/* 														   */
/* Verilen her satir(strip)'daki pikselleri color space ve */
/* image width degerine gore ekrana yazdirir.			   */   
/* ------------------------------------------------------- */
void displayPixels(char *str, int startIndex, long width, short colorSpace)
{
    int i = 0, j = 0;
    char pixels[8] = "", w = '\0', b = '\0';

    /* colorSpace = 0 ise beyazlar 0, siyahlar 1 olarak goruntulenir. */
    /*            = 1 ise beyazlar 1, siyahlar 0 olarak goruntulenir. */
    if(colorSpace == 0){
        w = '0';
        b = '1';
    }
    else{
        w = '1';
        b = '0';
    }

    /* pixels dizisinin icine str'den alinan hex. sayilarin binary karsiligini koy.           */
    /* str'den ornegin; ff gibi iki byte'lik hex. sayilari gelecegi icin pixels'i 8 karakter  */
    /* icin olusturmamiz yeterlidir.                                                          */
    while(str[j] != '\0'){
        switch(str[j]){
            case 'f': pixels[i] = w, pixels[i+1] = w, pixels[i+2] = w, pixels[i+3] = w; break;
            case 'e': pixels[i] = w, pixels[i+1] = w, pixels[i+2] = w, pixels[i+3] = b; break;
            case 'd': pixels[i] = w, pixels[i+1] = w, pixels[i+2] = b, pixels[i+3] = w; break;
            case 'c': pixels[i] = w, pixels[i+1] = w, pixels[i+2] = b, pixels[i+3] = b; break;
            case 'b': pixels[i] = w, pixels[i+1] = b, pixels[i+2] = w, pixels[i+3] = w; break;
            case 'a': pixels[i] = w, pixels[i+1] = b, pixels[i+2] = w, pixels[i+3] = b; break;
            case '9': pixels[i] = w, pixels[i+1] = b, pixels[i+2] = b, pixels[i+3] = w; break;
            case '8': pixels[i] = w, pixels[i+1] = b, pixels[i+2] = b, pixels[i+3] = b; break;
            case '7': pixels[i] = b, pixels[i+1] = w, pixels[i+2] = w, pixels[i+3] = w; break;
            case '6': pixels[i] = b, pixels[i+1] = w, pixels[i+2] = w, pixels[i+3] = b; break;
            case '5': pixels[i] = b, pixels[i+1] = w, pixels[i+2] = b, pixels[i+3] = w; break;
            case '4': pixels[i] = b, pixels[i+1] = w, pixels[i+2] = b, pixels[i+3] = b; break;
            case '3': pixels[i] = b, pixels[i+1] = b, pixels[i+2] = w, pixels[i+3] = w; break;
            case '2': pixels[i] = b, pixels[i+1] = b, pixels[i+2] = w, pixels[i+3] = b; break;
            case '1': pixels[i] = b, pixels[i+1] = b, pixels[i+2] = b, pixels[i+3] = w; break;
            case '0': pixels[i] = b, pixels[i+1] = b, pixels[i+2] = b, pixels[i+3] = b; break;
            default: printf("Wrong Character! %c\n", str[j]);
        } /* switch */
        
        i += 4;
        j++;
    } /* while */
  
    /* pixel dizisini yazdir.                                                   */
    /* startIndex parametresi ile kacinci piksel degerinde kaldigimizi anlariz. */
    /* Her piksel okundugunda da startIndex'i 1 arttiririz ki fazlalik bit'leri */
    /* kirpabilelim.                                                            */
    for(i = 0; i < 8; ++i){
        if(startIndex < width)
            printf("%c", pixels[i]);
        
        startIndex++;
    }    
}

/* readPixelsOfLine() 									   */
/* 														   */
/* Verilen tif dosyasindaki herhangi bir satir(strip)daki  */
/* hex. degerleri okur.	Bu degerler piksel degerleridir.   */
/* Elde ettigi hex degerlerini displayPixels fonksiyonuna  */
/* gonderir.                                               */
/*                                                         */
/* not: Okumalarda sadece siyah beyaz goruntuler icin      */
/*      teker byte ile pikseller alınacaktir. Bu yuzden    */
/*      byte order'a bu fonksiyonda ihtiyac duyulmaz.      */
/* ------------------------------------------------------- */
void readPixelsOfLine(FILE *fp, int line, long width, 
                      short bitsPerSample, short colorSpace)
{
    char *buffer;
    char str[8] = "";
    
    int i = 0;
    
    unsigned int bitsToRead = 0, byteValue = 0;

    /* buffer'i olustur ve initialize et. */
    buffer = (char  *) malloc(width * sizeof(char ));
    buffer[0] = '\0';
  
    /* En az image width kadar veya image width'ten daha fazla bit okumasi yapmamiz lazim. */
    /* Toplam kac bit okuma yapmamiz gerektigini hesapla.                                  */
    while(bitsToRead < width)
        bitsToRead += 8;

    /* Gerekli satir okumasini yap. */
    fread(buffer, 1, (bitsToRead / 8) * (bitsPerSample), fp);

    /* Bit olarak yapman gereken okumayi byte cinsinden ifade et ve okumalari */
    /* buffer'dan birer byte olarak gerceklestir.                             */
    for(i = 0; i < bitsToRead / 8; ++i){
    
        byteValue = buffer[i] & 0xff;
    
        sprintf(str, "%x", byteValue);
        /* Her byte okundugunda(dongu 1 kere dondugunde) 8 piksel ilerlenmis olacak. */
        /* Buffer'dan okunan byte'in binary karsilgini(pikselleri) yazdir.           */
    
        displayPixels(str, 8*i, width, colorSpace);
    }
    printf("\n");
  
    free(buffer);
}

/* getTiffPixels() 											   */
/*															   */
/* Verilen tif dosyasinin tum strip'lerindeki pikselleri okur. */
/* ----------------------------------------------------------- */
void getTiffPixels(char fileName[], char byteOrder, long length,
                   long width, long stripOffset, short bitsPerSample, short colorSpace)
{
    FILE  *fp;
    int i = 0;

    fp = fopen(fileName, "rb");
    
    if(fp != NULL){
        /* Her satirin piksellerini strip'lerden okuyacagiz.    */
        /* Bu yuzden strip'lere ulasmak icin dosyada ilerleriz. */
        fseek(fp, stripOffset, SEEK_SET);

        /* Her satirdaki(strip) piksel degerlerini oku. */
        for(i=0; i<length; i++)
            readPixelsOfLine(fp, i, width, bitsPerSample, colorSpace);

        fclose(fp);
    }
    else{
        printf("The file could not be opened.\n");
        printf("Errno: %d\n", errno);
    }
}

int main(int argc, char *argv[])
{
    long length = 0, width = 0, stripOffset= 0;
    
    char byteOrder = '\0';
    
    short bitsPerSample = 0, colorSpace = 0; 
    
    if(argc>2) {
        printf("Too many arguments supplied.\n");
    }
    else if(argc == 1){
        printf("One argument expected.\n");
    }

    getHeaderPart(argv[1], &byteOrder, &length, &width, &stripOffset, &bitsPerSample, &colorSpace);
    
    printf("Width : %lu pixels\n", width);
    printf("Length : %lu pixels\n", length);
    if(byteOrder == 'M')
        printf("Byte order : Motorola\n");
    else
        printf("Byte order : Intel\n");

    /* printf("bitsPerSample: %lu\n", bitsPerSample); */
    /* printf("stripOffset: %lu\n", stripOffset); */
    /* printf("colorSpace: %d\n", colorSpace); */

    getTiffPixels(argv[1], byteOrder, length, width, stripOffset, bitsPerSample, colorSpace);

    return 0;
}