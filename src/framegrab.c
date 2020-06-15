#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <ev.h>
#include <fcntl.h>


#define MAX_PIPES_PER_STREAM 10

#define CHANNEL_MAINSTREAM 0
#define CHANNEL_SUBSTREAM 1

//#define ENABLE_DEBUG_LOG

struct ev_loop_struct {
  //the async handlers libev provides to communicate between threads.
  unsigned long update_settings;
  unsigned long process_data;
  unsigned long cleanup; 
};

void (*ptr_process_video_frame)(uint ,int ,int ,unsigned long );
void (*ptr_process_audio_frame)(int ,int ,unsigned long );

static struct ev_loop * ev_loop_Ptr= 0;
static unsigned long hdl_VideoMainstream= 0;
static unsigned long hdl_VideoSubstream= 0;
static unsigned long hdl_AudioIn= 0;
static unsigned long hdl_AudioPlay= 0;

static unsigned long DAT_0003d350= 0;
static unsigned long DAT_0003d354= 0;
static unsigned long DAT_0003d34c= 0;
static unsigned long DAT_0003d348= 0;

static unsigned long DAT_0003d48c= 0;
static unsigned long DAT_0003d488= 0;
static unsigned long DAT_0003d484= 0;
 
static unsigned long VideoSubFrameLostCntr= 0;
static unsigned long VideoMainFrameLostCntr= 0;
static unsigned long audioFrameLostCntr= 0;
static unsigned long iSubIFrameWaitCounter= 0;
static unsigned long iMainIFrameWaitCounter= 0;
static unsigned long LastMainFrameNo= 0;
static unsigned long LastSubFrameNo= 0;
static unsigned long LastAudioFrameNo= 0;
static unsigned long iSubFrameWaitCounter= 0;
static unsigned long LastMainFrameTime= 0;
static unsigned long LastSubFrameTime= 0;
static unsigned long LastAudioFrameTime= 0;

static unsigned long StartTime= 0;
static unsigned long reqTime;
static unsigned long hdlBuffer;

static char cThreadRunning = 0;

static int mainstreamCounter = 0, substreamCounter = 0, audioPipeCounter = 0;
static FILE* mainstreamPipes[MAX_PIPES_PER_STREAM];
static FILE* substreamPipes[MAX_PIPES_PER_STREAM];
static FILE* audioPipes[MAX_PIPES_PER_STREAM];


void process_audio_frame(int ptrFrame,int ptrFramePayload,unsigned long frame_size)
{
	
#ifdef ENABLE_DEBUG_LOG
	printf("got audio frame frame_size=%d\n", frame_size);
#endif
	
	for(int i=0; i<audioPipeCounter; i++) {
		write(audioPipes[i], ptrFramePayload, frame_size*sizeof(char));
	}
	
}

void process_video_frame(uint videoChannel,int ptrFrame,int ptrFramePayload,unsigned long frame_size)
{
	
#ifdef ENABLE_DEBUG_LOG
	printf("got frame %d frame_size=%d\n",videoChannel, frame_size);
#endif

	if(videoChannel == 0){
		for(int i=0; i<mainstreamCounter; i++) {
			write(mainstreamPipes[i], ptrFramePayload, frame_size*sizeof(char));
		}
	}
	
	if(videoChannel == 1){
		for(int i=0; i<substreamCounter; i++) {
			write(substreamPipes[i], ptrFramePayload, frame_size*sizeof(char));
		}
	}
}

void ForceKeyFrame(int iParm1)

{
  int iVar1;
  time_t tVar2;
  size_t sVar3;
  
  tVar2 = time((time_t *)0x0);
  if (0 < tVar2 - reqTime) {
    reqTime = time((time_t *)0x0);
    if (iParm1 == 0) {
      hdlBuffer = hdl_VideoMainstream;
    }
    else {
      if (iParm1 == 1) {
        hdlBuffer = hdl_VideoSubstream;
      }
    }
    if (hdlBuffer == 0) {
#ifdef ENABLE_DEBUG_LOG		
      printf("shbf msg handle: %d have not been created!\n",iParm1);
#endif
    }
    else {
      tVar2 = time((time_t *)0x0);
#ifdef ENABLE_DEBUG_LOG
      printf("ForceKeyFrame msg send, chn: %d, time %d\n",iParm1,tVar2);
#endif
      iVar1 = hdlBuffer;
      sVar3 = strlen("REQ_IDR");
      shbfev_rcv_send_message(iVar1,"REQ_IDR",sVar3);
    }
  }
  return;
}


unsigned long on_recv_audio_stream(unsigned long uParm1,int *piParm2)

{
  int iVar1;
  unsigned long uVar2;
  int iVar3;
  time_t tVar4;
  int iVar5;
  unsigned long uVar6;
  int iVar7;
  
  if (piParm2 == (int *)0x0) {
    #ifdef ENABLE_DEBUG_LOG
	printf("on_recv_audio_stream err audio buf is NULL\n");
	#endif
    uVar6 = 0xffffffff;
  }
  else {
    iVar3 = shbf_get_size(piParm2);
    uVar2 = VideoSubFrameLostCntr;
    uVar6 = VideoMainFrameLostCntr;
    iVar1 = LastAudioFrameNo;
    if (iVar3 + -0x20 < 1) {
	  #ifdef ENABLE_DEBUG_LOG
      printf("on_recv_audio_stream err audio size is 0\n");
	  #endif
      shbf_free(piParm2);
      uVar6 = 0xffffffff;
    }
    else {
      if (0 < *piParm2) {
        if ((*piParm2 - LastAudioFrameNo != 1) && (LastAudioFrameNo != -1)) {
          iVar5 = (*piParm2 - LastAudioFrameNo) + audioFrameLostCntr + -1;
          iVar7 = *piParm2;
          audioFrameLostCntr = iVar5;
          tVar4 = time((time_t *)0x0);
		  #ifdef ENABLE_DEBUG_LOG
          printf(
                   "audio frame lost: cur fream no: %d last frame no: %d, main/sub/audio lost cnt:[%d/%d/%d], app run time:[%d]\n"
                   ,iVar7,iVar1,uVar6,uVar2,iVar5,tVar4 - StartTime);
		   #endif
        }
        LastAudioFrameNo = *piParm2;
      }
      LastAudioFrameTime = time((time_t *)0x0);
      (*ptr_process_audio_frame)(piParm2,piParm2 + 8,iVar3 + -0x20);
      shbf_free(piParm2);
      uVar6 = 0;
    }
  }
  return uVar6;
}


unsigned long on_recv_video_stream(int* piParm1,int iParm2)
{ 
  int iVar1 	 = 0;
  int iVar2 	 = 0;
  time_t tVar3	 = 0;
  int iVar4		 = 0;
  int iSubStream = 0;
  unsigned long uVar5 = 0;
  unsigned long uVar6 = 0;
#ifdef ENABLE_DEBUG_LOG
  printf("on_recv_video_stream: ");
#endif
  if (iParm2 == 0) {
#ifdef ENABLE_DEBUG_LOG
    printf("on_recv_video_stream err video buf is NULL\n");
#endif
    uVar5 = 0xffffffff;
  }
  else {
    iVar2 = shbf_get_size(iParm2);
    if (iVar2 + -0x20 < 1) {
#ifdef ENABLE_DEBUG_LOG
      printf("on_recv_video_stream err, size is zero\n");
#endif
      shbf_free(iParm2);
      uVar5 = 0;
    }
    else {
      iSubStream = *piParm1;
      if (iSubStream == 0) {
        if (iMainIFrameWaitCounter < 1) {
          if ((*(int *)(iParm2 + 4) - LastMainFrameNo != 1) && (LastMainFrameNo != -1)) {
            if (iMainIFrameWaitCounter == 0) {
              iMainIFrameWaitCounter = 1;
              ForceKeyFrame(0);
            }
            uVar5 = audioFrameLostCntr;
            iVar1 = VideoSubFrameLostCntr;
            iVar2 = LastMainFrameNo;
            iVar4 = (*(int *)(iParm2 + 4) - LastMainFrameNo) + VideoMainFrameLostCntr + -1;
            uVar6 = *(unsigned long *)(iParm2 + 4);
            VideoMainFrameLostCntr = iVar4;
            tVar3 = time((time_t *)0x0);
#ifdef ENABLE_DEBUG_LOG
            printf(
                     "main stream frame lost: cur frame no: %d last frame no: %d, main/sub/audiolost cnt: [%d/%d/%d], app run time:[%d]\n"
                     ,uVar6,iVar2,iVar4,iVar1,uVar5,tVar3 - StartTime);
#endif
            LastMainFrameNo = *(unsigned long *)(iParm2 + 4);
            shbf_free(iParm2);
            return 0xffffffff;
          }
        }
        else {
          if (*(int *)(iParm2 + 0x14) != 1) {
#ifdef ENABLE_DEBUG_LOG
            printf("main Waitting for next I frame: %d\n",iMainIFrameWaitCounter);
#endif
            iMainIFrameWaitCounter = iMainIFrameWaitCounter + 1;
            LastMainFrameNo = *(unsigned long *)(iParm2 + 4);
            shbf_free(iParm2);
            return 0;
          }		  

#ifdef ENABLE_DEBUG_LOG
          printf("main waiting for I frame fin, wait cnt: %d\n",iMainIFrameWaitCounter);
#endif
          iMainIFrameWaitCounter = 0;
        }
        LastMainFrameNo = *(int *)(iParm2 + 4);
        LastMainFrameTime = time((time_t *)0x0);
      }
      else {
        if (iSubStream == 1) {
          if (iSubFrameWaitCounter < 1) {
            if ((*(int *)(iParm2 + 4) - LastSubFrameNo != 1) && (LastSubFrameNo != -1)) {
              if (iSubFrameWaitCounter == 0) {
                iSubFrameWaitCounter = 1;
                ForceKeyFrame(1);
              }
              uVar5 = audioFrameLostCntr;
              iVar1 = VideoMainFrameLostCntr;
              iVar2 = LastSubFrameNo;
              iVar4 = (*(int *)(iParm2 + 4) - LastSubFrameNo) + VideoSubFrameLostCntr + -1;
              uVar6 = *(unsigned long *)(iParm2 + 4);
              VideoSubFrameLostCntr = iVar4;
              tVar3 = time((time_t *)0x0);
#ifdef ENABLE_DEBUG_LOG
              printf(
                       "sub stream frame lost: cur fream no: %d last frame no: %d, main/sub/audiolost cnt: [%d/%d/%d], app run time: [%d]\n"
                       ,uVar6,iVar2,iVar1,iVar4,uVar5,tVar3 - StartTime);
#endif
              LastSubFrameNo = *(unsigned long *)(iParm2 + 4);
              shbf_free(iParm2);
              return 0xffffffff;
            }
          }
          else {
            if (*(int *)(iParm2 + 0x14) != 1) {
#ifdef ENABLE_DEBUG_LOG
              printf("sub Waitting for next I frame: %d\n",iSubFrameWaitCounter);
#endif
              iSubFrameWaitCounter = iSubFrameWaitCounter + 1;
              LastSubFrameNo = *(unsigned long *)(iParm2 + 4);
              shbf_free(iParm2);
              return 0;
            }
#ifdef ENABLE_DEBUG_LOG
            printf("sub waiting for I frame fin, wait cnt: %d\n",iSubFrameWaitCounter);
#endif
            iSubFrameWaitCounter = 0;
          }
          LastSubFrameNo = *(int *)(iParm2 + 4);
          LastSubFrameTime = time((time_t *)0x0);
        }
      }
      (*ptr_process_video_frame)(iSubStream,iParm2,iParm2 + 0x20,iVar2 + -0x20);
      shbf_free(iParm2);
      uVar5 = 0;
    }
  }
  return uVar5;
}

void on_stream_start(int *piParm1)
{
  int iVar1;
  int iVar2;
  int iVar3;
  time_t tVar4;
  int iVar5;
  
  if (piParm1 != (int *)0x0) {
    iVar5 = *piParm1;
    if (iVar5 == 0) {
      DAT_0003d484 = DAT_0003d484 + 1;
    }
    else {
      if (iVar5 == 1) {
        DAT_0003d488 = DAT_0003d488 + 1;
      }
      else {
        if (iVar5 == 100) {
          DAT_0003d48c = DAT_0003d48c + 1;
        }
      }
    }
    iVar3 = DAT_0003d48c;
    iVar2 = DAT_0003d488;
    iVar1 = DAT_0003d484;
    tVar4 = time((time_t *)0x0);
#ifdef ENABLE_DEBUG_LOG
    printf("stream start: %d,  main/sub/audio stream reconn times:[ %d/%d/%d],  time:%d\n",iVar5,
             iVar1,iVar2,iVar3,tVar4);
#endif
  }
  DAT_0003d348 = 0;
  return;
}

void receiver_closed_callback(int *piParm1)
{
  time_t tVar1;
  int local_c;
  
  local_c = -1;
  if (piParm1 != (int *)0x0) {
    local_c = *piParm1;
    if (local_c == 0) {
      DAT_0003d350 = 0xffffffff;
    }
    else {
      if (local_c == 1) {
        DAT_0003d354 = 0xffffffff;
      }
      else {
        if (local_c == 100) {
          DAT_0003d34c = 0xffffffff;
        }
      }
    }
  }
  tVar1 = time((time_t *)0x0);
#ifdef ENABLE_DEBUG_LOG
  printf("%d receiver closed!time: %d\n",local_c,tVar1);
#endif
  DAT_0003d348 = 0;
  return;
}


void ev_cleanup_func(void)

{
  ev_break(ev_loop_Ptr,2);
  return;
}


void fetchstream_thread()

{
  pthread_t __th = 0;
  struct ev_loop_struct evs = {0,0,0};
  ev_timer local_38 = {0};
  int local_c = 0;
#ifdef ENABLE_DEBUG_LOG
   printf("start fetchstream_thread\n");
#endif
  __th = pthread_self();
  pthread_detach(__th);
  
  StartTime = time((time_t *)0x0);
  local_c = prctl(0xf,"getstream",0,0,0);
  if (local_c != 0) {
#ifdef ENABLE_DEBUG_LOG
    printf("prctl setname failed\n");
#endif
  }


  shbf_rcv_global_init();
  ev_loop_Ptr = ev_default_loop(0);
  unsigned long var0 = 0;
  unsigned long var1 = 1;
  unsigned long var2 = 2;
  unsigned long var3 = 3;
  unsigned long var4 = 4;
 


  ev_timer_again(ev_loop_Ptr,&local_38);
#ifdef ENABLE_DEBUG_LOG
  printf("ev_timer_again"); 
  printf("set ptr");
#endif
  ptr_process_video_frame = process_video_frame;
#ifdef ENABLE_DEBUG_LOG
  printf("ptr_process_video_frame");
#endif
  ptr_process_audio_frame = process_audio_frame;
  hdl_VideoMainstream = shbfev_rcv_create(ev_loop_Ptr,"/run/video_mainstream");
  shbfev_rcv_event(hdl_VideoMainstream,2,on_recv_video_stream,&var0);
  shbfev_rcv_event(hdl_VideoMainstream,1,receiver_closed_callback,&var0);
  shbfev_rcv_event(hdl_VideoMainstream,0,on_stream_start,&var0);
  shbfev_rcv_start(hdl_VideoMainstream);
#ifdef ENABLE_DEBUG_LOG
  printf("shbfev_rcv_start(hdl_VideoMainstream)");
#endif
  hdl_VideoSubstream = shbfev_rcv_create(ev_loop_Ptr,"/run/video_substream");
  shbfev_rcv_event(hdl_VideoSubstream,2,on_recv_video_stream,&var1);
  shbfev_rcv_event(hdl_VideoSubstream,1,receiver_closed_callback,&var1);
  shbfev_rcv_event(hdl_VideoSubstream,0,on_stream_start,&var1);
  shbfev_rcv_start(hdl_VideoSubstream);
#ifdef ENABLE_DEBUG_LOG
  printf("shbfev_rcv_start hdl_VideoSubstream");
#endif
  
  //no audio for now
  
  hdl_AudioIn = shbfev_rcv_create(ev_loop_Ptr,"/run/audio_in");
  shbfev_rcv_event(hdl_AudioIn,2,on_recv_audio_stream,&var2);
  shbfev_rcv_event(hdl_AudioIn,1,receiver_closed_callback,&var2);
  shbfev_rcv_event(hdl_AudioIn,0,on_stream_start,&var2);
  shbfev_rcv_start(hdl_AudioIn);
  //hdl_AudioPlay = shbfev_rcv_create(ev_loop_Ptr,"/run/audio_play");
  //shbfev_rcv_event(hdl_AudioPlay,1,receiver_closed_callback,&var3);
  //shbfev_rcv_event(hdl_AudioPlay,0,on_stream_start,&var3);
  //shbfev_rcv_start(hdl_AudioPlay);
  //DAT_0003f97c = 0;
  evs.update_settings = 0;
  evs.process_data = 0;
  evs.cleanup = ev_cleanup_func;
  
  ev_async_start(ev_loop_Ptr,&evs);
  ev_run(ev_loop_Ptr,0);
  ev_timer_again(ev_loop_Ptr,&local_38);
  shbfev_rcv_destroy(hdl_VideoMainstream);
  shbfev_rcv_destroy(hdl_VideoSubstream);
  shbfev_rcv_destroy(hdl_AudioIn);
  //shbfev_rcv_destroy(hdl_AudioPlay);
  shbf_rcv_global_exit();
  ev_loop_destroy(ev_loop_Ptr);
                   
  pthread_exit((void *)0xfffffffb);
}


void sig_handler(int signo)
{
#ifdef ENABLE_DEBUG_LOG
    if (signo == SIGUSR1)
        printf("received SIGUSR1\n");
    else if (signo == SIGKILL)
        printf("received SIGKILL\n");
    else if (signo == SIGSTOP)
        printf("received SIGSTOP\n");
    else  if (signo == SIGINT)
        printf("received SIGINT\n");
#endif

    cThreadRunning = 0;
}

void openPipe(char* path, FILE* pipes[], int channel, int* counter) {
	FILE* auxFile;
	auxFile = open(path, O_NONBLOCK | O_RDWR);
	if(auxFile == NULL) {
		printf("Can't open pipe '%s'\n", path);
		return NULL;
	}
	
	pipes[*counter] = auxFile;
	(*counter)++;
}

void closePipes(FILE* pipes[], int counter) {
	for(int i=0; i<counter; i++){
		fclose(pipes[i]);
	}
}

int main(int argc, char** args)
{
  pthread_t pStack16 = 0;
  int local_c = 0;
  int opt;
 
  while ((opt = getopt (argc, args, "m:s:a:")) != -1)
  {
	  switch(opt) {
		  case 'm':
			openPipe(optarg, mainstreamPipes, CHANNEL_MAINSTREAM, &mainstreamCounter);
		  break;		 
		  case 's':
			openPipe(optarg, substreamPipes, CHANNEL_SUBSTREAM, &substreamCounter);
		  break;
		  case 'a':
			openPipe(optarg, audioPipes, 0, &audioPipeCounter);
			break;
	  }
  }
  
#ifdef ENABLE_DEBUG_LOG  
  printf("start main\n");
#endif

  local_c = pthread_create(&pStack16,(pthread_attr_t *)0x0,fetchstream_thread, (void *)0x0);
  
  if (local_c != 0) {
#ifdef ENABLE_DEBUG_LOG  
	printf("create_fetchstream_thread ret=%d\n",local_c);
#endif
	exit(-1);
  }
  
  if (signal(SIGUSR1, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGUSR1\n");
    if (signal(SIGKILL, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGKILL\n");
    if (signal(SIGSTOP, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGSTOP\n");
    if (signal(SIGINT, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGINT\n");
	
  cThreadRunning = 1;
  while(cThreadRunning){
     sleep(1);
  }
  
  closePipes(mainstreamPipes, mainstreamCounter);
  closePipes(substreamPipes, substreamCounter);
  closePipes(audioPipes, audioPipeCounter);


#ifdef ENABLE_DEBUG_LOG
  printf("exit %d",local_c );
#endif
}
