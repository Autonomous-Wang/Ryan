#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>

#define BUFSIZE 1248
#define SERVICE_PORT 2368
#define PI 3.1415926

// include the receive part in the main function and first build the linked list
struct node
{
	double stmp;  // store second in integer
	//FILE *packet; // store xyz parsed data and intensity in the file accessed by file pointer
	int id;
	struct node *next;
} *head = NULL, *tail = NULL; 

struct frame
{
	double stmp;
	//FILE *frame;
    int id;
	struct frame *next;
} *head_f = NULL, *tail_f = NULL;

// this function to combine file into one whose time stamp within 0.1 seconds
void appendfile(int num, int frame_num) // append file f2 at the end of frame file
{
    char filename[20], framename[20];  
    sprintf(filename, "#%dpacket.txt", num); 
    sprintf(framename, "#%dframe.txt", frame_num);

    FILE *fr = fopen(framename, "a");
    FILE *f = fopen(filename, "r");
    char buffer[50]; // read each line in the frame as string
    if(f == NULL) 
    {
        perror("Error opening file.");
    }
    else 
    {
        while(fgets(buffer, sizeof(buffer), f)) 
        {
        fprintf(fr, "%s", buffer);
        }
        //fprintf(fr, "\n");
    }
    fclose(f);
    fclose(fr);
}

// check all the timestamp in the linked list and frame at given position
void checklist(struct node *head)
{
    printf("%d %lf\n", head->id, head->stmp);
    if (head->next == NULL)
    {
        return;
    }
    checklist(head->next);
}

// for calculation in parse packet file
int hex(int num1, int num2)
{
    int y;
    y = num1*16*16+num2;
    return y;
}

// for finding the head pointer for new frame
struct node* move(struct node *head, int num)
{
    int i;
    for (i = 1; i < num; i++)
    {
        head = head->next;
    }
    return head;
}

// parsing the packet received
void parse(int num, FILE *ptr_myfile) // num is used for naming the parsed data
{
    int counter;
    unsigned char buff[1206];
    unsigned int x[1206];
    if (!ptr_myfile)
    {
        printf("Unable to open file!");
        return NULL;
    }
    fread(buff, 1, sizeof(buff), ptr_myfile);
    for (counter = 0; counter < sizeof(buff); counter++)
    {
        x[counter] = buff[counter];
    }
    fclose(ptr_myfile);
    
    double omega[32] = {-15,1,-13,-3,-11,5,-9,7,-7,9,-5,11,-3,13,-1,15,-15,1,-13,-3,-11,5,-9,7,-7,9,-5,11,-3,13,-1,15};
    // tansfer into pi
    int i1;
    for (i1=0; i1<32; i1++)
    {
        omega[i1]=omega[i1]*PI/180;
    }
    double Azimuth[2][12];
    double Distance_channel[32];
    double X[384];
    double Y[384];
    double Z[384];
    unsigned int Inten[384];
    double Datablock[100];
    double Azimuth1, Azimuth2, Distance1, Distance2;

    double diff;
    int j;
    int k;
    int m;
    for (j=0; j<12; j++)
    {
        for (k=0; k<100; k++)
        {
            Datablock[k] = x[100*j+k];
        }
        Azimuth1 = Datablock[3];
        Azimuth2 = Datablock[2];
        Azimuth[0][j] = hex(Azimuth1, Azimuth2)/100*PI/180;
    }
    diff = Azimuth[0][11] - Azimuth[0][0];
    if (diff < 0)
    {
        diff = (diff + 2*PI)/11;
    }
    else
    {
        diff = diff/11.0;
    }
    for (j=0; j<11; j++)
    {
        if (Azimuth[0][j] > Azimuth[0][j+1])
       {
        Azimuth[1][j] = (Azimuth[0][j] + Azimuth[0][j+1]+2*PI)/2;
       }
       else
       {
        Azimuth[1][j] = (Azimuth[0][j] + Azimuth[0][j+1])/2.0;
       }
       if (Azimuth[1][j] > 2*PI)
       {
        Azimuth[1][j] = Azimuth[1][j] -2*PI;
       }
    }    

    Azimuth[1][11] = (Azimuth[0][0] + Azimuth[0][11] + diff)/2;
    if (Azimuth[1][11] > 2*PI)
    {
        Azimuth[1][11]= Azimuth[1][11]-2*PI;
    }
        // Distance calculation
        for (j=0; j<12; j++)
        {
            for (k=0; k<100; k++)
           {
                Datablock[k] = x[100*j+k];
            }
            for (m=0; m<32; m++)
           {   
            // distance calculation
            Distance1 = Datablock[m*3+5];
            Distance2 = Datablock[m*3+4];
            Distance_channel[m] = hex(Distance1, Distance2)*2/1000.0;

            // store intensity value
            Inten[32*j+m] = Datablock[m*3+6];
            
            // from sphere to xyz coordinate
                if (m < 16)
                {
                    X[32*j+m]=Distance_channel[m]*cos(omega[m])*sin(Azimuth[0][j]);
                    Y[32*j+m]=Distance_channel[m]*cos(omega[m])*cos(Azimuth[0][j]);
                    Z[32*j+m]=Distance_channel[m]*sin(omega[m]);
                }
                else
                {
                    X[32*j+m]=Distance_channel[m]*cos(omega[m])*sin(Azimuth[1][j]);
                    Y[32*j+m]=Distance_channel[m]*cos(omega[m])*cos(Azimuth[1][j]);
                    Z[32*j+m]=Distance_channel[m]*sin(omega[m]);
                }
            }
        }
    // change X, Y, Z into matrix and return it to main function
    // use the defined unrolled linked list to store X, Y and Z coordinate
    int n;
    // write processed data to file and plot and this is not necessary needed.
    FILE *temp;
    char filename[20]; 
    sprintf(filename, "#%dpacket.txt", num); 
    temp = fopen(filename, "w+");
    // FILE * gnuplotPipe = popen ("gnuplot -persistent", "w");
    for (n = 0; n < 384; n++)
    {
        // range can also be specified in the following statement, to filter out unecessary points
        if (!(X[n]==0 && Y[n]==0 && Z[n] == 0)) // remove all zero values and can also add filter
        {
            fprintf(temp, "%lf %lf %lf %u\n", X[n], Y[n], Z[n], Inten[n]);
        }
    }
    //fprintf(gnuplotPipe, "%s \n", "splot 'XYZ.txt' with points pointtype 7 pointsize 0.8");
    //fclose(gnuplotPipe);
    fclose(temp);
 }

// plot the data in one parsed packet
void showpacket(int num)
{
    char filename[20];
    sprintf(filename, "#%dpacket.txt", num); 
    FILE * gnuplotPipe = popen ("gnuplot -persistent", "w");
    fprintf(gnuplotPipe, "%s \n", "splot 'filename' with points pointtype 7 pointsize 0.8");
    fclose(gnuplotPipe);
}

double timestmp(FILE *ptr_myfile)
{
    // getting the time stamp byte
    int counter, i;
    unsigned char buff[1206];
    unsigned int x[1206];
    unsigned int time[4];
    double stamp; // in microseconds
    if (!ptr_myfile)
    {
        printf("Unable to open file!");
        return 1;
    }
    fread(buff, 1, 1206, ptr_myfile);
    for (counter = 0; counter < 1206; counter++)
    {
        x[counter] = buff[counter];
    }
    fclose(ptr_myfile);
    // timestamp is stored as four byte values
    for (i = 0; i < 4; i++)
    {
        time[i] = x[i + 1200]; // store in reverse order to process it easier
    }
    // transfer the byte value into integer as micro-sencond
    stamp = (time[3]*256*256*256 + time[2]*256*256 + time[1]*256 + time[0]*1)/1000000.0; 
    return stamp;
}
// loadData("example.dat"); printData(); dbScan(); printData();
