#include <mictcp.h>
#include <api/mictcp_core.h>   
#include<pthread.h> 
#define MAX_TRANS 20 
#define TAILLE_Fen 5 
#define TAUX 0.2     // Max 1 perte sur 5 
int PE=0; 
int PA=0;   
unsigned long timeout=50; //en ms  
pthread_mutex_t mutex= PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t cond=PTHREAD_COND_INITIALIZER;




int t[TAILLE_Fen]={1,1,1,1,1};   //fenêtre de taille 5 



mic_tcp_sock socketClient;


/*
 * Permet de calculer le taux des pertes
 */
float tauxp(int *t) {   
    int s=0;
    for (int i=0; i<5; i++) {  
        s+=t[i];
    } 
    return (float)(5-s)/5;
}
/*
 * Permet de remplir de tab des pertes
 */
void updateTabp(int *t, int ack) {   

    for (int i=0; i<TAILLE_Fen-1; i++) {  
        t[i]=t[i+1];
    } 
    t[4] = ack; 
}

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
    int result = -1;
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    result = initialize_components(sm); /* Appel obligatoire */
    if (result==-1) {  return -1 ;}
    set_loss_rate(5);
    socketClient.state= IDLE; 
    socketClient.fd =0 ;
    return socketClient.fd;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   return 1;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");  
    if (pthread_mutex_lock(&mutex)!=0){printf("Erreur mutex lock \n ");}   
    socketClient.state=WAIT_SYN; 
    if (pthread_mutex_unlock(&mutex)!=0){printf("Erreur mutex lock \n ");}  
    if (pthread_mutex_lock(&mutex)!=0){printf("Erreur mutex lock \n ");}  
    while(socketClient.state==WAIT_SYN){ 
        pthread_cond_wait(&cond,&mutex);
    }  
    if (pthread_mutex_unlock(&mutex)!=0){printf("Erreur mutex lock \n ");} 
    struct mic_tcp_header header; 
    struct mic_tcp_payload payload;  
    header.source_port = 1234;
    header.dest_port = socket ; 
    header.seq_num = PE; //PE
    header.ack_num = 0;
    header.syn = '1';
    header.ack = '1';
    header.fin = '0';   
    payload.data = NULL;
    payload.size = 0;  
    struct mic_tcp_pdu pdu ;
    pdu.header= header;
    pdu.payload = payload;  
    int taille=IP_send(pdu, *addr);  
    if (taille==-1){ 
        printf("Erreur IP_SEND")  ;  }  
     if (pthread_mutex_lock(&mutex)!=0){printf("Erreur mutex lock \n ");}  
    while(socketClient.state==WAIT_ACK){ 
        pthread_cond_wait(&cond,&mutex);
    }  
    if (pthread_mutex_unlock(&mutex)!=0){printf("Erreur mutex lock \n ");}
    
    return 1;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    if (socketClient.state == IDLE) { 
        struct mic_tcp_header header; 
        struct mic_tcp_payload payload;  
        header.source_port = 1234;
        header.dest_port = socket ; 
        header.seq_num = PE; //PE
        header.ack_num = 0;
        header.syn = '1';
        header.ack = '0';
        header.fin = '0';   
        payload.data = NULL;
        payload.size = 0;  
        int attempt=0; 
        int reussi=0;
        struct mic_tcp_pdu pdu ;
        pdu.header= header;
        pdu.payload = payload;  
          


         while ((attempt<MAX_TRANS)&&(reussi==0)) {  
            int taille=IP_send(pdu, addr); 
            if (taille==-1) { 
            printf("Erreur dans le IP_SEND \n"); 
            exit(-1);
        }    
            socketClient.state=WAIT_SYNACK;
            struct mic_tcp_pdu Recep = {0};
            int tailleRecue = IP_recv(&Recep, &addr,50);   
            printf ("tailleReçue = %d \n", tailleRecue);   
            if ( tailleRecue == -1) {
                printf("Timeout : no ack \n ");   
                attempt ++; 
                taille=IP_send(pdu, addr);
                } 
            else{
                    if ((Recep.header.ack == '1') && (Recep.header.syn=='1')) { 
                        reussi=1; 

                    } 
                    header.syn = '1';
                    header.ack = '1';  
                    pdu.header= header;
                    pdu.payload = payload;   
                    taille=IP_send(pdu, addr); 
                    if (taille==-1) { 
                    printf("Erreur dans le IP_SEND \n"); 
                    exit(-1);
                    }  
                    socketClient.state =ESTABLISHED;    
                    return 0;

        
            }
        } 
      
    } 
    return -1;
    
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{ 
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n"); 
    struct mic_tcp_sock_addr addr;  
    addr.ip_addr="127.0.0.1";  
    addr.ip_addr_size = sizeof(addr.ip_addr);
    addr.port = mic_sock; 
    struct mic_tcp_header header; 
    struct mic_tcp_payload payload;  
    header.source_port = 1234;
    header.dest_port = mic_sock ; 
    header.seq_num = PE; //PE
    header.ack_num = 0;
    header.syn = '0';
    header.ack = '0';
    header.fin = '0';    
    int reussi=0;
    payload.data = mesg;
    payload.size = mesg_size; 
    struct mic_tcp_pdu pdu ;
    pdu.header= header;
    pdu.payload = payload; 
    int taille = -1;
    int attempt = 0; 
    PE=(PE+1)%2;
    struct mic_tcp_pdu Recep = {0};
    int tailleRecue; 
   
    if (socketClient.state == ESTABLISHED) { 
        taille=IP_send(pdu, addr);  
        socketClient.state=WAIT_ACK;


        if (taille==-1) { 
            printf("Erreur dans le IP_SEND \n"); 
            exit(-1);
        }    
        if (taille>=0){ printf("Message envoyé \n "); }

        

        while ((attempt<MAX_TRANS)&&(reussi==0)) {  
            tailleRecue = IP_recv(&Recep, &addr,50);
            printf ("tailleReçue = %d \n", tailleRecue); 
            
            if ( tailleRecue == -1) {
                printf("Timeout : no ack \n ");
                printf("Taux pertes : ----%f \n ", tauxp(t));
                if (tauxp(t)>TAUX){
                            attempt ++; 
                            taille=IP_send(pdu, addr);  
                            //updateTabp(t,0);
                            
                } else { //perte admise
                    printf("perte admise \n"); 
                    reussi = 1;
                    updateTabp(t,0);
                }
                
               
            } else {
                    
                    if ((Recep.header.ack == '1') && (Recep.header.ack_num == PE)) {
                        reussi=1;  
                        /*if (attempt ==0) { 
                          updateTabp(&t,1);
                        }
                        else {
                            for(int i=0 ; i<TAILLE_Fen;i++) t[i]=1;
                        } */
                        updateTabp(t,1);
                        printf(" apres recv PA = %d Pe= %d \n ",Recep.header.ack_num, PE);
                        break; 
                    } /*else {
                        
                        if (tauxp(t)>TAUX){ 
                            updateTabp(t,0); 
                            attempt ++; 
                            taille=IP_send(pdu, addr); 
                            if (taille==-1) { 
                                printf("Erreur dans le IP_SEND \n"); 
                                exit(-1);
                            }   
                        } else { updateTabp(t,1); reussi=1; } */
                     
            }
            if (attempt == MAX_TRANS) {
                updateTabp(t,0);
            }
            printf("Attempt send  n°= %d \n", attempt); 
            
            
        } 
        if (attempt>MAX_TRANS) { 
            taille=-1; 
        }

        for(int i=0; i<5;i++) printf("          t %d = %d \n",i,t[i]);

    } 
    socketClient.state=ESTABLISHED;
    return taille ;
    
}


    


/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n"); 
   
    mic_tcp_payload payload;
    payload.size =  max_mesg_size;
    payload.data = mesg;
    int taille = app_buffer_get(payload);
    
    return taille;
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    return -1;
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr)
{   
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n"); 
    if (socketClient.state==WAIT_SYN) {  
        if(pdu.header.syn=='1'){ 
        if (pthread_cond_broadcast(&cond)!=0){ 
            printf("Erreur pthread cond broadcast \n ");
        } 
    if (pthread_mutex_lock(&mutex)!=0){printf("Erreur mutex lock \n ");} 
    socketClient.state=WAIT_ACK;  
    if (pthread_mutex_unlock(&mutex)!=0){printf("Erreur mutex lock \n ");}
     
    } }
    else if (socketClient.state==WAIT_ACK){   
        if (pdu.header.ack=='1'){ 
             if (pthread_cond_broadcast(&cond)!=0){ 
            printf("Erreur pthread cond broadcast \n ");
        }   if (pthread_mutex_lock(&mutex)!=0){printf("Erreur mutex lock \n ");}
            socketClient.state=ESTABLISHED; 
            if (pthread_mutex_unlock(&mutex)!=0){printf("Erreur mutex lock \n ");}
        }  

    } 
    else{ 
    struct mic_tcp_header header;
    header.source_port = 1234;
    header.dest_port = pdu.header.source_port; 
    header.seq_num = 0; 
    header.ack_num = PA;
    header.syn = '0';
    header.ack = '1';
    header.fin = '0';    
    struct mic_tcp_pdu pdu1;
    pdu1.header= header; 
    pdu1.payload.data = NULL; 
    pdu1.payload.size = 0; 
        
    if(pdu.header.seq_num==PA) {  
        printf("Message envoyé \n"); 
        
        PA=(PA+1)%2;  
        pdu1.header.ack_num = PA;
        printf("Recepteur : PA %d, Seq_num %d\n", PA, pdu.header.seq_num); 

        app_buffer_put(pdu.payload);   
           
    } 
    printf("Recepteur : ack_num %d, \n", pdu1.header.ack_num); 
    
    
    int taille=IP_send(pdu1, addr);
    printf("TAILLE %d\n", taille);
    if (taille==-1) { 
        printf("Erreur dans le IP_SEND \n"); 
        exit(1);
    } 
        
    
    }   


}

//gbenalay@insa-toulouse.fr
        

