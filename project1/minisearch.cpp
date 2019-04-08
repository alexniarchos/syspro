//minisearch.cpp

#include <iostream>
#include "string.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "trie.h"
#include "commands.h"
#include "globals.h"

using namespace std;

//GLOBAL VARIABLES
int linecounter = 0;
int k = 10;

int main(int argc, char* argv[]){
    int argnum;

    // cout << "argc = " << argc << endl;
    // for(int i=0;i<argc;i++){
    //     cout << "arg[" << i << "] = " << argv[i] << endl;
    // }
    //check arguments
    if(argc < 2){
        cout << "not enough arguments" << endl;
        return 0;
    }
    if(strcmp(argv[1],"-i") == 0){// ./minisearch -i docfile -k 5
        if(argc > 2 && strcmp(argv[2],"") != 0){
            cout << "Filename: " << argv[2] << endl; 
            argnum = 2;
        }
        else{
            cout << "filename argument required!" << endl;
            return 0;
        }
        if(argc>=4 && strcmp(argv[3],"-k") == 0){
            if(argc == 5){
                k = atoi(argv[4]);
                cout << "k = " << k << endl;
            }
            else{
                cout << "flag -k without value" << endl;
                return 0;
            }
        }
    }
    else if(argc > 2 && strcmp(argv[1],"-k") == 0){// ./minisearch -k (5) -i docfile
        k = atoi(argv[2]);
        if(argc > 4 && strcmp(argv[3],"-i") == 0){
            if(strcmp(argv[4],"") != 0){
                cout << "Filename: " << argv[4] << endl; 
                argnum = 4;
            }
            else{
                cout << "filename argument required!" << endl;
                return 0;
            }
        }
        else{
            cout << "not enough arguments" << endl;
            return 0;
        }
    }
    else{
        cout << "not enough arguments" << endl;
        return 0;
    }
    //Document counting
    char c;
    int maxlettercount=0,lastlettercount=0;
    FILE *file;
    file = fopen (argv[argnum],"r");
    if (file != NULL)
    {
        while((c=fgetc(file)) != EOF){
            lastlettercount++;
            if(c == '\n'){
                linecounter++;
                if(maxlettercount < lastlettercount){
                    maxlettercount = lastlettercount;
                }
                lastlettercount = 0;
            }
        }
        if(maxlettercount < lastlettercount){
            maxlettercount = lastlettercount;
        }
        cout << "linecounter: " << ++linecounter << endl;
        fclose (file);
    }
    else{
        cout << "Input file is empty!!!" << endl;
        return 0;
    }
    char* docs[linecounter];
    for(int i=0;i<linecounter;i++){
        docs[i] = NULL;
    }
    file = fopen (argv[argnum],"r");
    if (file != NULL){
        int i=0,nread = 0;
        size_t len = 0;
        char id[512];
        int previd=-1;
        for(int i=0;i<linecounter;i++){
            fscanf(file,"%s",id);
            //eat tabs/spaces between id and text
            while(!feof(file)) {
                c = getc (file);
                if( c != ' ' && c != '\t' ) {
                    ungetc (c, file);
                    break;
                }
            }
            if((nread = getline(&docs[i], &len, file)) != -1){
                for(int k=strlen(docs[i])-1;k>=0;k--){
                    if(docs[i][k] == ' ' || docs[i][k] == '\t' || docs[i][k] == '\n'){
                        docs[i][k] = '\0';
                    }
                    else break;
                }
                if(++previd != atoi(id)){
                    cout << "Documents are NOT in the right order" << endl;
                    return 0;
                }
            }
        }
        fclose(file);
    }

    //insert each token to the Trie
    //cout << "maxlettercount = " << maxlettercount << endl;
    char *tok,buf[maxlettercount+1];
    trienode *root = new trienode();
    root->letter = -2;
    for(int i=0;i<linecounter;i++){
        strcpy(buf,docs[i]);
        tok = strtok(buf," \t\r\n");
        while(tok != NULL){
            //insert tok into trie
            insertTrie(root,tok,i);
            tok = strtok(NULL," \t\r\n");
        }
    }
    //waiting for commands like /search /df /tf
    char *cmd = NULL,*args;
    size_t len = 0;
    while(1){
        cout << "syspro:/>";
        if((getline(&cmd, &len, stdin) != -1) && (strcmp(cmd,"\n")!=0)){
            //cout << cmd << endl;
            tok = strtok(cmd," \t\r\n");
            if(strcmp(tok,"/df")==0){
                //cout << "executing /df" << endl;
                tok = strtok(NULL," \t\r\n");
                if(tok == NULL){
                    df(root,NULL);
                }
                while(tok!=NULL){
                    df(root,tok);
                    tok = strtok(NULL," \t\r\n");
                }
            }
            else if(strcmp(tok,"/tf")==0){
                //cout << "executing /tf" << endl;
                tok = strtok(NULL," \t\r\n");
                char* tok2 = strtok(NULL," \t\r\n");
                if(tok == NULL || tok2 == NULL){
                    cout << "not enough arguments" << endl;
                }
                else{
                    int termfreq = tf(atoi(tok),tok2,root);
                    if(termfreq != 0){
                        cout << tok2 << " " << termfreq << endl;
                    }
                }
            }
            else if(strcmp(tok,"/search")==0){
                //cout << "executing /search" << endl;
                int i=0;
                char *words[10];
                for(i=0;i<10;i++){
                    words[i] = strtok(NULL," \t\r\n");
                    if(words[i]==NULL){ 
                        break;
                    }
                }
                search(root,docs,words,i);
            }
            else if(strcmp(tok,"/exit")==0){
                //cout << "Exiting commands" << endl;
                //Free MAP
                for(int i=0;i<linecounter;i++){
                    free(docs[i]);
                }
                freeTrie(root);
                free(cmd);
                return 0;
            }
            else{
                cout << "Wrong command!!!" << endl;
            }
        }
        free(cmd);
        cmd = NULL;
    }
    
    cout << "Exiting main..." << endl;
}
