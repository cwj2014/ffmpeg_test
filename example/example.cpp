extern void AVOption_Example(); 
extern void AVDictionary_Example();
extern void AVLog_Example();
extern void AVRational_Example();
extern void AVBuffer_Example();
extern void AVSample_Example();
extern void AVFifoBuffer_Example();
extern void AVAudioFifo_Example();
extern void AVFrame_Example();
extern void AVMath_Example();
//////////////////////////////////////////////
extern void AVFormatInput_Example();
extern void AVStream_Example();
int main(){
    AVOption_Example();
    AVDictionary_Example();
    AVLog_Example();
    AVRational_Example();
    AVBuffer_Example();
    AVSample_Example();
    AVFifoBuffer_Example();
    AVAudioFifo_Example();
    AVFrame_Example();
    AVMath_Example();

    AVFormatInput_Example();
    AVStream_Example();
    return 0;
}