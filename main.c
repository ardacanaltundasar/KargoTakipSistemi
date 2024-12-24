#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STACK_SIZE 5
// Kargo göndermek için önce kullanıcı bilgisi ve şehir bilgisi verileri kullanıcı tarafından girilmelidir. 
// Sonrasında kargo gönderme işlemi yapılabilir.
typedef struct Kargo {
    int id;
    char tarih[11]; // YYYY-MM-DD formatında tarih
    int teslim_durumu; // 0 = Teslim Edilmedi, 1 = Teslim Edildi
    int teslim_suresi; // Teslim Süresi (Gün)
    struct Kargo *sonraki;
    struct Sehir* sehir; // Şehir
} Kargo;

typedef struct Musteri {
    int id;
    char ad[50];
    char soyad[50];
    Kargo *kargo_gecmisi;
    struct Musteri *sonraki;
} Musteri;

typedef struct Yigin {
    Kargo *kargolar[MAX_STACK_SIZE];
    int ust;
} Yigin;

typedef struct Sehir {
    int id;
    char ad[50];
    struct Sehir* sol; 
    struct Sehir* sag; 
} Sehir;

Musteri *musteri_listesi = NULL; 
Kargo *pq_oncelik = NULL; 
Yigin *yigin = NULL; 

// 1. Müşteri Verilerini Yönetme

// Yeni bir kargo yaratma
Kargo* kargo_olustur(int id, const char* tarih, int teslim_durumu, int teslim_suresi, Sehir* sehir) {
    Kargo* yeni_kargo = (Kargo*)malloc(sizeof(Kargo));
    yeni_kargo->id = id;
    strcpy(yeni_kargo->tarih, tarih);
    yeni_kargo->teslim_durumu = teslim_durumu;
    yeni_kargo->teslim_suresi = teslim_suresi;
    yeni_kargo->sehir = sehir;  
    yeni_kargo->sonraki = NULL;
    return yeni_kargo;
}

// Yeni bir müşteri yaratma
Musteri* musteri_olustur(int id, const char* ad, const char* soyad) {
    Musteri* yeni_musteri = (Musteri*)malloc(sizeof(Musteri));
    yeni_musteri->id = id;
    strcpy(yeni_musteri->ad, ad);
    strcpy(yeni_musteri->soyad, soyad);
    yeni_musteri->kargo_gecmisi = NULL;
    yeni_musteri->sonraki = NULL;
    return yeni_musteri;
}

// Yeni bir müşteri ekleme
void musteri_ekle(int id, const char* ad, const char* soyad) {
    Musteri* yeni_musteri = musteri_olustur(id, ad, soyad);
    yeni_musteri->sonraki = musteri_listesi;
    musteri_listesi = yeni_musteri;
}

// Kargo geçmişini sıralı şekilde ekleme
void kargo_ekle_musteriye(int musteri_id, int kargo_id, const char* tarih, int teslim_durumu, int teslim_suresi, Sehir* sehir) {
    Musteri* current = musteri_listesi;
    while (current != NULL) {
        if (current->id == musteri_id) {
            Kargo* yeni_kargo = kargo_olustur(kargo_id, tarih, teslim_durumu, teslim_suresi, sehir);
            // Bağlantılı listeye tarih sırasına göre ekleme
            if (current->kargo_gecmisi == NULL || strcmp(yeni_kargo->tarih, current->kargo_gecmisi->tarih) < 0) {
                yeni_kargo->sonraki = current->kargo_gecmisi;
                current->kargo_gecmisi = yeni_kargo;
            } else {
                Kargo* temp = current->kargo_gecmisi;
                while (temp->sonraki != NULL && strcmp(temp->sonraki->tarih, yeni_kargo->tarih) < 0) {
                    temp = temp->sonraki;
                }
                yeni_kargo->sonraki = temp->sonraki;
                temp->sonraki = yeni_kargo;
            }
            return;
        }
        current = current->sonraki;
    }
    printf("Musteri bulunamadi!\n");
}

// Müşteri geçmişini yazdırma
void musteri_gecmisi_yazdir(int musteri_id) {
    Musteri* current = musteri_listesi;
    while (current != NULL) {
        if (current->id == musteri_id) {
            printf("%s %s icin Kargo Gecmisi:\n", current->ad, current->soyad);
            Kargo* kargo = current->kargo_gecmisi;
            while (kargo != NULL) {
                printf("Kargo ID: %d, Tarih: %s, Durum: %d, Teslim Suresi: %d gun, Sehir: %s\n",
                    kargo->id, kargo->tarih, kargo->teslim_durumu, kargo->teslim_suresi, kargo->sehir->ad);
                kargo = kargo->sonraki;
            }
            return;
        }
        current = current->sonraki;
    }
    printf("Musteri bulunamadi!\n");
}

// 2. Kargo Önceliklendirme (Priority Queue)

Kargo* kargo_olustur_oncelik(int id, int teslim_suresi, int durum, Sehir* sehir) {
    Kargo* yeni_kargo = (Kargo*)malloc(sizeof(Kargo));
    yeni_kargo->id = id;
    yeni_kargo->teslim_suresi = teslim_suresi;
    yeni_kargo->teslim_durumu = durum;
    yeni_kargo->sehir = sehir;  
    yeni_kargo->sonraki = NULL;
    return yeni_kargo;
}

// Kargo öncelikli kuyrukta sıralı ekleme
void kuyruga_ekle(int id, int teslim_suresi, int durum, Sehir* sehir) {
    Kargo* yeni_kargo = kargo_olustur_oncelik(id, teslim_suresi, durum, sehir);
    if (pq_oncelik == NULL || pq_oncelik->teslim_suresi > teslim_suresi) {
        yeni_kargo->sonraki = pq_oncelik;
        pq_oncelik = yeni_kargo;
    } else {
        Kargo* temp = pq_oncelik;
        while (temp->sonraki != NULL && temp->sonraki->teslim_suresi <= teslim_suresi) {
            temp = temp->sonraki;
        }
        yeni_kargo->sonraki = temp->sonraki;
        temp->sonraki = yeni_kargo;
    }
}

// 3. Kargo Rotalama (Tree Data Structure)

// Şehir oluşturma
Sehir* sehir_olustur(int id, const char* ad) {
    Sehir* yeni_sehir = (Sehir*)malloc(sizeof(Sehir));
    yeni_sehir->id = id;
    strcpy(yeni_sehir->ad, ad);
    yeni_sehir->sol = yeni_sehir->sag = NULL;
    return yeni_sehir;
}

// Şehir ekleme (sıralı şekilde)
Sehir* sehri_ekle(Sehir* root, int id, const char* ad) {
    if (root == NULL) return sehir_olustur(id, ad);
    if (id < root->id) {
        root->sol = sehri_ekle(root->sol, id, ad);
    } else {
        root->sag = sehri_ekle(root->sag, id, ad);
    }
    return root;
}

// Şehirleri sıralı yazdırma
void sehirleri_yazdir(Sehir* root) {
    if (root == NULL) return;
    sehirleri_yazdir(root->sol);
    printf("Sehir ID: %d, Adi: %s\n", root->id, root->ad);
    sehirleri_yazdir(root->sag);
}

// 4. Gönderim Geçmişi Sorgulama (Stack)

void yigin_baslat(Yigin* yigin) {
    yigin->ust = -1;
}

void yigina_ekle(Yigin* yigin, Kargo* kargo) {
    if (yigin->ust == MAX_STACK_SIZE - 1) {
        printf("Yigin dolu!\n");
    } else {
        yigin->kargolar[++yigin->ust] = kargo;
    }
}

Kargo* yigindan_cikart(Yigin* yigin) {
    if (yigin->ust == -1) {
        printf("Yigin bos!\n");
        return NULL;
    }
    return yigin->kargolar[yigin->ust--];
}

void yigin_yazdir(Yigin* yigin) {
    if (yigin->ust == -1) {
        printf("Yigin bos!\n");
        return;
    }
    printf("Son 5 Gönderim:\n");
    for (int i = yigin->ust; i >= 0; i--) {
        printf("Kargo ID: %d\n", yigin->kargolar[i]->id);
    }
}

// Kullanıcıdan veri almak için yardımcı fonksiyonlar
void string_giris_al(char* mesaj, char* input) {
    printf("%s", mesaj);
    fgets(input, 50, stdin);
    input[strcspn(input, "\n")] = '\0'; // Yeni satırı kaldır
}

int int_giris_al(char* mesaj) {
    int deger;
    printf("%s", mesaj);
    scanf("%d", &deger);
    getchar(); // yeni satır karakterini temizle
    return deger;
}

int main() {
    int secim;
    Sehir* sehir_root = NULL;

    while(1) {
        printf("\n--- Kargo Takip Sistemi ---\n");
        printf("1. Yeni musteri ekle\n");
        printf("2. Kargo gonderimi ekle\n");
        printf("3. Kargo gecmisini sorgula\n");
        printf("4. Tum musterileri listele\n");
        printf("5. Yeni sehir ekle\n");
        printf("6. Sehirleri listele\n");
        printf("7. Cik\n");
        printf("Seciminizi yapin: ");
        scanf("%d", &secim);
        getchar();

        if (secim == 1) {
            int id = int_giris_al("Musteri ID girin: ");
            char ad[50], soyad[50];
            string_giris_al("Musteri adi girin: ", ad);
            string_giris_al("Musteri soyadi girin: ", soyad);
            musteri_ekle(id, ad, soyad);
        }
        else if (secim == 2) {
            int musteri_id = int_giris_al("Musteri ID girin: ");
            int kargo_id = int_giris_al("Kargo ID girin: ");
            char tarih[11];
            string_giris_al("Kargo gonderim tarihini girin (YYYY-MM-DD): ", tarih);
            int teslim_durumu = int_giris_al("Teslim durumu (0 = Teslim edilmedi, 1 = Teslim edildi): ");
            int teslim_suresi = int_giris_al("Teslim suresi (Gun olarak): ");
            
            printf("Sehirler:\n");
            sehirleri_yazdir(sehir_root);
            int sehir_id = int_giris_al("Kargo gonderilecegi sehri secin (ID): ");
            
            Sehir* sehir = NULL;
            Sehir* temp = sehir_root;
            while (temp != NULL) {
                if (temp->id == sehir_id) {
                    sehir = temp;
                    break;
                }
                temp = temp->sag;
            }
            
            if (sehir == NULL) {
                printf("Secilen sehir bulunamadi!\n");
            } else {
                kargo_ekle_musteriye(musteri_id, kargo_id, tarih, teslim_durumu, teslim_suresi, sehir);
            }
        }
        else if (secim == 3) {
            int musteri_id = int_giris_al("Musteri ID girin: ");
            musteri_gecmisi_yazdir(musteri_id);
        }
        else if (secim == 4) {
            Musteri* current = musteri_listesi;
            while (current != NULL) {
                printf("Musteri ID: %d, Ad: %s, Soyad: %s\n", current->id, current->ad, current->soyad);
                current = current->sonraki;
            }
        }
        else if (secim == 5) {
            int sehir_id = int_giris_al("Sehir ID girin: ");
            char sehir_ad[50];
            string_giris_al("Sehir adı girin: ", sehir_ad);
            sehir_root = sehri_ekle(sehir_root, sehir_id, sehir_ad);
        }
        else if (secim == 6) {
            printf("Sehirler:\n");
            sehirleri_yazdir(sehir_root);
        }
        else if (secim == 7) {
            break;
        } else {
            printf("Gecersiz secenek. Lutfen tekrar deneyin.\n");
        }
    }
    return 0;
}
