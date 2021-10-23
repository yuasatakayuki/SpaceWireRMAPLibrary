#ifndef SPACEWIRESPACEPIMODULE_HH_
#define SPACEWIRESPACEPIMODULE_HH_

#include "CxxUtilities/Exception.hh"
#include <wiringPi.h>
#include <bcm2835.h>
#include <sys/time.h>
#include <iostream>
#include <pthread.h>

#define TIMEOUT_MILLISEC_DEFAULT 1000

//#define DebugSpaceWireSpacePiModule
#undef DebugSpaceWireSpacePiModule

using namespace std;

// dummy function, now interrupt is checked with polling
void fpga_int(){
}

class SpaceWireSpacePiException: public CxxUtilities::Exception{
    public:
        enum{
            DataSizeTooLarge,
            Disconnected,
            Timeout,
            Undefined,
        };

    public:
        SpaceWireSpacePiException(uint32_t status):
            CxxUtilities::Exception(status){
            }

    public:
        virtual ~SpaceWireSpacePiException(){}

    public:
        string toString(){
            string result;
            switch(status){
                case DataSizeTooLarge:
                    result = "DataSizeTooLarge";
                    break;
                case Disconnected:
                    result = "Disconnected";
                    break;
                case Timeout:
                    result = "Timeout";
                    break;
                case Undefined:
                    result = "Undefined";
                    break;

                default:
                    result = "Undefined status";
                    break;
            }
            return result;
        }
};

class SpaceWireSpacePiModule {

    enum EOPType {
        EOP = 0x00, EEP = 0x01, Undefined = 0xffff
    };

    public:
    SpaceWireSpacePiModule(){
    };

    ~SpaceWireSpacePiModule(){};

    long timeout_duration_millisec;
    pthread_mutex_t mutex;

    public:
    int32_t open(void){
        // SPI初期化
        if ( !bcm2835_init()) return -1;

        bcm2835_spi_begin();
        bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
        bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
        bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);
        bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
        bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

        // interrupt
        int ret;
        ret = wiringPiSetupSys();
        if ( ret < 0 ) {
            printf("Interrupt setup error\n");
            return ret;
        }else {
            wiringPiISR( 27, INT_EDGE_RISING, fpga_int );		// GPIO27 interrupt setting
            pinMode(27, INPUT);
            // printf("Interrupt setup OK\n");
        }

        timeout_duration_millisec = TIMEOUT_MILLISEC_DEFAULT;
        pthread_mutex_init(&mutex, NULL);

        return 0;
    }

    public:
    void close(void){
        // TODO:割り込み処理の停止

        // TODO:ハンドラスレッドの終了

        // SPIの終了
        bcm2835_spi_end();
        bcm2835_close();
    }

    /*
     * SpacePi の送信バッファへbufのデータを送信する
     * 
     * uint8_t *buf 送信データバッファ
     * uint32_t len 送信データ長
     * uint32_t type 送信データの終端の種類（）
     */
    public:
    //extern int32_t spacepi_send(uint8_t *buf, size_t len, enum EOPType type);
    int32_t send(uint8_t *buf, size_t len){

        enum EOPType type = EOP;  //TODO
        uint8_t SpiTxData[2048];	// SPI送信データ


        // SPI フォーマット（Cmd, Address）
        char tx_sts_read[] = { 0x20, 0x00, 0x00, 0x00 };  // リード、Txバッファステータス
        char tx_snd_eop[] = { 0xA0, 0x02, 0x01, 0x00 };	  // ライト、Txバッファコントロール、EOP送信コマンド
        char tx_snd_eep[] = { 0xA0, 0x02, 0x03, 0x00 };	  // ライト、Txバッファコントロール、EEP送信コマンド

        pthread_mutex_lock(&mutex);
        spi_transfer(&tx_sts_read[0], sizeof(tx_sts_read)); // Read Tx buffer status

        //debug
        unsigned int tx_buf_cap = (((unsigned int)tx_sts_read[3] << 8) & 0x0F00) | ((unsigned int)tx_sts_read[2]);
#ifdef DebugSpaceWireSpacePiModule
        printf("SpacePiModule::send() tx_sts_read[3]=0x%02X, [2]=0x%02X\n",tx_sts_read[3], tx_sts_read[2]);
        if(tx_sts_read[3] != 0x08){
            int i;
            for(i=0;i<len;++i){
                printf("%02X ", buf[i]);
            }
            printf("\n");
        }
#endif

        // Tx buffer empty
        if (tx_buf_cap == 0x800) {

            spi_cre_write_packet(&SpiTxData[0], 0x0000, len, buf);

#if 0
            int cnt;
            printf("len=%d\n",len);
            for ( cnt=0; cnt<len; cnt++ ) {
                printf("%02X ",SpiTxData[cnt]);
                if ((cnt%16)==15 ) printf("\n");
            }
            printf("\n");
#endif

            // Write SpaceWire data to Tx Buffer
#ifdef DebugSpaceWireSpacePiModule
            printf("SpacePiModule::send() Sending data to Tx buffer...\n");
#endif
            spi_transfer(&SpiTxData[0], len+2);

            // Write SpaceWire end of packet to Tx buffer control
#ifdef DebugSpaceWireSpacePiModule
            printf("SpacePiModule::send() SPI send command...\n");
#endif
            if(type == EEP) {
                spi_transfer(&tx_snd_eep[0], sizeof(tx_snd_eop));
            } else {
                spi_transfer(&tx_snd_eop[0], sizeof(tx_snd_eop));
            }
            pthread_mutex_unlock(&mutex);
#ifdef DebugSpaceWireSpacePiModule
            printf("SpacePiModule::send() SPI send finished\n");
#endif

        } else {
            printf("SpacePiModule::send() SpacePi TxBuffer is not empty.\n");
            printf("SpacePiModule::send() Tx buffer cap: %d/2048 [byte]\n", tx_buf_cap);
            return -1;
        }

        return 0;

    }

    /*
     *
     * @ [OUT]rx_data　
     * @ [INOUT]rx_size IN: 呼び出し元が確保したサイズ、　OUT：SpacePiから格納したサイズ
     *
     */
    public:
    //extern int spacepi_receive(uint8_t *buf, size_t len, enum type);
    int receive(uint8_t *rx_data, size_t *rx_size) throw (SpaceWireSpacePiException) {
        char buf[16];
        const char rx_sts_rd[] = { 0x20, 0x10, 0x00, 0x00 };		// Rxバッファステータス読込み
        const char tx_int_clr[] = { 0xA0, 0x12, 0x01, 0x00 };		// Txバッファ 受信割込みクリア
        const char* packet_end[] = {"EOP","EEP","Continue","Reserved"};
        size_t size;
        struct timeval tv_start, tv_now;

        int rcv_flg = 1;
        long elapsed_time = 0;

#ifdef DebugSpaceWireSpacePiModule
        printf("SpacePiModule::receive() entered.\n");
#endif
        gettimeofday(&tv_start, NULL);

        while(rcv_flg) {

            usleep(1);
            gettimeofday(&tv_now, NULL);
            long dt_us = (tv_now.tv_sec - tv_start.tv_sec) * 1000000 + (tv_now.tv_usec - tv_start.tv_usec);

            if(dt_us > timeout_duration_millisec * 1000){
                *rx_size = 0;
                throw SpaceWireSpacePiException(SpaceWireSpacePiException::Timeout);
            }

            // FPGA割込みチェック
            if ( digitalRead(27) ) {						// 受信ありのとき

                memcpy(&buf[0], &rx_sts_rd[0], 4);
                pthread_mutex_lock(&mutex);
                spi_transfer(&buf[0], 4);			// 受信ステータス取得

                char rx_packet_end = ((buf[3]&0xC0)>>6)&0x03;
                // 受信サイズを格納
                size = ((short)buf[3]<<8) & 0x0F00;
                size = size | (buf[2] & 0xFF);
#ifdef DebugSpaceWireSpacePiModule
                printf("SpacePiModule::receive() pkt with %s received: %d [byte]\n",packet_end[rx_packet_end], size);
#endif

                if ((buf[3]&0xC0) == 0 ) {
                    // EOPパケット受信のとき

                    // sizeが確保したバッファサイズよりも小さい場合は、sizeの大きさに変更する
                    if(size < *rx_size) {
                        *rx_size = size;
                    }

                    // 受信データを確保
                    memset(&rx_data[0], 0x00, *rx_size+3);
                    rx_data[0] = 0x08;								// 0x0800=Recive Buffer
#ifdef DebugSpaceWireSpacePiModule
                    printf("SpacePiModule::receive() SPI getting Rx buffer...\n");
#endif
                    spi_transfer(&rx_data[0], *rx_size+2);
#ifdef DebugSpaceWireSpacePiModule
                    printf("SpacePiModule::receive() SPI getting Rx buffer Done\n");
#endif
                }else{
                    //EOP以外のとき
                    printf("SpacePiModule: Invalid Packet Received!!\n");
                }

                // SPI Interrupt clear
#ifdef DebugSpaceWireSpacePiModule
                printf("SpacePiModule::receive() Clearing intterupt flag...\n");
#endif
                memcpy(&buf[0], &tx_int_clr[0], 4);
                spi_transfer(&buf[0], 4);		// 割込みクリア
                pthread_mutex_unlock(&mutex);
#ifdef DebugSpaceWireSpacePiModule
                printf("SpacePiModule::receive() Clearing intterupt flag done.\n");
#endif

                rcv_flg = 0;
            }
        }

        //int cnt;
        //printf("rx_size=%d\n",*rx_size);
        //for ( cnt=0; cnt<*rx_size; cnt++ ) {
        //	printf("%02X ",rx_data[cnt]);
        //	if ((cnt%16)==15)printf("\n");
        //}
        //printf("\n");

        return 0;

    }


    // internal function
    private:
    void spi_transfer(void *buf, uint32_t len){

        bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
        bcm2835_spi_transfern((char *)buf, len);

    }

    /*
     * SPIの書込みコマンド作成
     */
    private:
    int32_t spi_cre_write_packet(uint8_t *buf, uint16_t addr, size_t length, uint8_t *write_data)
    {
        if ( length > 2046 ) return -1;

        addr = addr | 0x8000;		// Command(Write)
        buf[0] = (addr >> 8) & 0xFF;
        buf[1] = addr & 0xFF;

        memcpy( &buf[2], write_data, length);

        length = 2 + length;			// Command + Address = 2 + read length

        return 0;
    }

    /*
     * SPIの読込みコマンド作成
     */
    private:
    uint32_t spi_cre_read_packet(uint8_t *buf, uint16_t addr, uint32_t length)
    {
        memset(&buf[0], 0x00, 2048);

        addr = addr & 0x7FFF;				// Command(Read)
        buf[0] = (addr >> 8) & 0xFF;
        buf[1]  = addr & 0xFF;

        length = 2 + length;		// Command + Address = 2 + read length

        return 0;
    }

    /*
     * SPI readのタイムアウト設定
     */
    public:
    void setTimeout(long millisecond){
        timeout_duration_millisec = millisecond;
    }
};


#endif 
