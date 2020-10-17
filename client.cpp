#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
#include <thread>
using namespace std;

FIFORequestChannel* create_new_channel(FIFORequestChannel* chan) {
    char name[1024];
    MESSAGE_TYPE m = NEWCHANNEL_MSG;
    chan->cwrite(&m, sizeof(m));
    chan->cread(name, 1024);
    FIFORequestChannel* newchan = new FIFORequestChannel(name, FIFORequestChannel::CLIENT_SIDE);
    return newchan;
}

void patient_thread_function(int n, int pno, FIFORequestChannel* chan, HistogramCollection* hc){
    /* What will the patient threads do? */
    datamsg d(pno, 0.0, 1);
    double resp = 0;
    for (int i = 0; i < n; i++) {
        chan->cwrite(&d, sizeof(datamsg));
        chan->cread (&resp, sizeof(double)) ;
        hc->update(pno, resp);
        d.seconds += 0.004;
    }
}

void worker_thread_function(/*add necessary arguments*/){
    /*
		Functionality of the worker threads	
    */
}



int main(int argc, char *argv[])
{
    int n = 1000;    //default number of requests per "patient"
    int p = 10;     // number of patients [1,15]
    int w = 100;    //default number of worker threads
    int b = 20; 	// default capacity of the request buffer, you should change this default
	int m = MAX_MESSAGE; 	// default capacity of the message buffer
    srand(time_t(NULL));
    
    int pid = fork();
    if (pid == 0){
		// modify this to pass along m
        execl ("server", "server", (char *)NULL);
    }
    
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
	HistogramCollection hc;
	
    // Creating Histograms for HistogramCollection
	for (int i = 0; i < p; i++) {
        Histogram* h = new Histogram(10, -2.0, 2.0);
        hc.add(h);
    }
	

    FIFORequestChannel* wchans[p];
    for (int i = 0; i < p; i++) {
        wchans[i] = create_new_channel (chan);
    }

    struct timeval start, end;
    gettimeofday (&start, 0);

    /* Start all threads here */
	thread patient[p];
    for (int i = 0; i < p; i++) {
        patient[i] = thread (patient_thread_function, n, i+1, wchans[i], &hc);
    }
	/* Join all threads here */
    for (int i = 0; i < p; i++) {
        patient[i].join();
    }
    gettimeofday (&end, 0);
    // print the results
	hc.print ();
    // cleans up worker channels
    for (int i = 0; i < p; i++) {
        MESSAGE_TYPE q = QUIT_MSG;
        wchans[i]->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
        delete wchans[i];
    }
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;
    
}
