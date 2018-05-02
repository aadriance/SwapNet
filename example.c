#include<sys/socket.h> 
#include<netinet/in.h> 
#include<stdio.h> 
#include<string.h>
#include<stdlib.h> 
#include <arpa/inet.h>
#include <unistd.h>

main()
{ 
    char buf[100]; 
    socklen_t len; 
    int k,sock_desc,temp_sock_desc; 
    struct sockaddr_in client,server; 
    memset(&client,0,sizeof(client)); 
    memset(&server,0,sizeof(server)); 
    sock_desc = socket(AF_INET,SOCK_STREAM,0); 
    server.sin_family = AF_INET; server.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    server.sin_port = 7777; 
    printf("Example binding\n");
    k = bind(sock_desc,(struct sockaddr*)&server,sizeof(server)); 
    k = listen(sock_desc,20); len = sizeof(client);
    temp_sock_desc = accept(sock_desc,(struct sockaddr*)&client,&len); 
    while(1)
    {     
        k = recv(temp_sock_desc,buf,100,0);     
        if(strcmp(buf,"exit")==0)        
            break;     
        if(k>0)         
            printf("%s",buf); 
    } close(sock_desc); 
    close(temp_sock_desc); 
    return 0; 
}