#include<iostream>
#include<stdlib.h>
#include<omp.h>
#include <mpi.h>
#include<unistd.h>
using namespace std;

int main(int argc, char* argv[])
{
    int myrank,nprocs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    //Master Proccess
    if(myrank == 0){
    
        cout<<"Name: Talha Mustafa"<<endl;
	cout<<"Roll Number: 18I-0573"<<endl;

        int size = 20, search;
        int *arr = new int[size];

        //Initializing array with odd numbers(as given in pdf)
        for(int i = 0; i < size; i++){
            arr[i] = (i * 2) + 1;
        }

        //Prining master(proccess 0) array
        cout<<endl<<"Proccess "<<myrank<<" has input data: ";
        for(int i = 0; i < size; i++){
               cout<<arr[i];
               if(i < size - 1){
                   cout<<", ";
               }
        }
        cout<<endl;
        sleep(1);
        
        //Number to search
        search = 39;
        cout<<endl<<"Master: The Number to Search is: "<<search<<endl;
        
        //Dividing the array in equal parts
        #pragma omp parallel num_threads(nprocs - 1)
        {
            bool first = true;
            int div_size = 0, div_index = 0;

            //div_index gives starting index to send to every local array
            //div_size gives the size of local arrays
            #pragma omp for schedule(static)
                for(int i = 0; i < size; i++){
                    if(first){
                        div_index = i;
                        first = false;
                    }
                    div_size++;
                }

                //Sending local size, local array and number to search to other proccesses
                MPI_Send(&div_size, 1, MPI_INT, omp_get_thread_num() + 1, 1234, MPI_COMM_WORLD);
                MPI_Send(&arr[div_index], div_size, MPI_INT, omp_get_thread_num() + 1, 1234, MPI_COMM_WORLD);
                MPI_Send(&search, 1, MPI_INT, omp_get_thread_num() + 1, 1234, MPI_COMM_WORLD);
        }

        //Waiting for reply from other proccess(Number found/ Not found)
        int found;
        MPI_Recv(&found, 1, MPI_INT, MPI_ANY_SOURCE, 1233, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        //If Number not found
        if(found == 999){
            cout<<endl<<"No one found the Number."<<endl;
        }
        else{
            //If number found
            cout<<endl<<"Master: Proccess "<<found<<" has found the number!"<<endl;
            cout<<"Master: Informing all proccesses to abort!"<<endl<<endl;
        }
        //Master proccess informing other proccess to abort
        for(int i = 1; i < nprocs; i++){
            MPI_Send(&found, 1, MPI_INT, i, 1232, MPI_COMM_WORLD);
        }
        
    }
    else{
        //Slave Proccesses.

        int div_size, search;

        //Recieve size of local array
        MPI_Recv(&div_size, 1, MPI_INT, 0, 1234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        int *arr2 = new int[div_size];

        //Recieve local array
        MPI_Recv(arr2, div_size, MPI_INT, 0, 1234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        //Recieve number to search
        MPI_Recv(&search, 1, MPI_INT, 0, 1234, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        //Printing local arrays
        cout<<"Proccess "<<myrank<<" has local data: ";
        for(int i = 0; i < div_size; i++){
               cout<<arr2[i];
               if(i < div_size - 1){
                   cout<<", ";
               }
        }
        cout<<endl;
        sleep(1);
        
        bool signal = false, found = false;

        //Multiple threads for every proccess, one waits on recieve, one searches number
        #pragma omp parallel num_threads(2) shared(signal, found)
        {
            //Waiting for abort signal
            if(omp_get_thread_num() == 0){
                int found;
                MPI_Recv(&found, 1, MPI_INT, 0, 1232, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                //Global signal to abort remaining proccesses.
                signal = true;
            }
            else{

                //Searching number
                for(int i; i < div_size; i++){
                    //If number found, aborting remaining proccesses.
                    if(signal){ 
                        cout<<"Proccess "<<myrank<<" Aborting."<<endl;
                        break;
                    }
                    //If number found, return proccess number to master.
                    if(arr2[i] == search){
                        found = true;
                        cout<<endl<<"Proccess "<<myrank<<": I have found the number."<<endl;
                        MPI_Send(&myrank, 1, MPI_INT, 0, 1233, MPI_COMM_WORLD);
                        break;
                    }
                    //Sleep to check the aborting conditions. Sleep every proccess except the one that has number.
                    //In this case it was in last proccess
                    if(myrank != nprocs - 1){
                        sleep(2);
                    }
                    
                }
                //If number not found in any proccess. Sending not found signal to master
                sleep(nprocs);
                if(!found){
                    int not_found = 999;
                    MPI_Send(&not_found, 1, MPI_INT, 0, 1233, MPI_COMM_WORLD);
                }
            }
        }
    }
    MPI_Finalize();
	return 0;
}