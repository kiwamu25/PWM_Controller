/* 
 * File:   Main.c
 * Author: K.Ishigame
 *
 * Created on 2019/05/31, 21:43
 * Updated on 2019/06/17, 20:12
 */
#include "pic.h"

/* ソフトウェアPWM、調節VR、ONOFFスイッチ各４回路用プログラム
 * Compiler : hi-tech-picc(v9.83)
 * Device : PIC16F1827
 * MPLAB X IDE v5.20
 */


__CONFIG(FOSC_INTOSC & WDTE_OFF & PWRTE_ON & MCLRE_OFF & CP_OFF & CPD_OFF & BOREN_OFF & CLKOUTEN_OFF & IESO_OFF & FCMEN_OFF);
__CONFIG(WRT_OFF & PLLEN_OFF & STVREN_OFF & BORV_LO & LVP_OFF);

//====PWM最大幅 変更時はADRESHのシフト数(AD_Shift_Count)も変更すること====
//0.5us * TMR0=255 * Width_Count=32 約4ms=250Hz?
#define Width_Count 32 //PWMの最大幅と調光ステップ数
#define AD_Shift_Count 3 //0=255 ,1=128 , 2=64 , 3=32 , 4=16

//関数宣言
static void pic_init();
static void wait_us(unsigned char);
static void AdConverter();
static void PWM_LED();

//変数宣言
static unsigned char P_Width[4]; //PWMのON幅設定値用 回路数分(4つ)
static unsigned char P_counter[4]; //PWMのON幅現在値用 同じく回路数分(4つ)
static unsigned char TotalWidth; //PWMの最大幅用

//割り込み
static void interrupt intr(void) {

    if (T0IF) {
        PWM_LED();
        TMR0 = 0; //TMR0の初期化
        T0IF = 0; //TMR0割込みの初期化

    }

}

static void PWM_LED() {
    unsigned char i; //for文ループ用
    unsigned char status = 0; //出力ポートの状態格納用

    //TotalWidthが「0」になるまで引く
    if (TotalWidth--) {
        for (i = 0; i < 4; i++) {
            //P_counterが0か確認 0なら何もしない
            if (P_counter[i] != 0) {
                //0じゃなかったら
                //P_counterの値を「0」になるまで引く
                P_counter[i]--;
                //bitをシフトすることでPORTの位置を決める
                status = status + (1 << i);
            }
        }

    } else {
        //0になったら

        //TotalWidthの初期化
        TotalWidth = Width_Count;

        //各P_counterの初期化
        for (i = 0; i < 4; i++) {
            P_counter[i] = P_Width[i];
        }

    }
    //LED出力
    PORTB = (status << 4);
}

void main() {
    //PICの初期化
    pic_init();
    
    //ポーリング
    while (1) {
        AdConverter();
    }
}

static void AdConverter() {
    unsigned char i;
    for (i = 0; i < 4; i++) {
        //安定化用（必要性不明なんとなく入れた）
        wait_us(40);

        //カウンターをシフトしてAN0-3までを切り替える + ADON = 1
        ADCON0 = 0x01 | (i << 2);
        // acquisition time　アクィジション？タイム待ち
        wait_us(40);

        //変換開始
        GO_nDONE = 1;
        while (GO_nDONE);
        //SW(RB0~RB3)がON(LOW)ならADCONの結果を格納するOFF(HIGH)なら0を代入
        if (!(PORTB & (0x01 << i))) {
            //Width_Count(PWM最大幅)の値に合わせてシフトして代入
            P_Width[i] = ADRESH >> AD_Shift_Count;
        } else {
            P_Width[i] = 0;
        }
        
        //AD変換終了
        ADON = 0;
    }
}

static void pic_init() {
    OSCCON = 0x72; //8Mhz 内臓オシレーター PLL未使用
    OPTION_REG = 0x08;
    ANSELA = 0x0F; //ADCON設定 AN0-3使用
    ANSELB = 0x00; //ADCON設定 不使用
    TRISA = 0xFF; //PORTAはすべてInput
    TRISB = 0x0F; //PORTB　0-3:Input 4-7:Output
    LATA = 0xFF; //ラッチ？よくわからないのでTRISと一緒
    LATB = 0x0F; //ラッチ？よくわからないのでTRISと一緒
    WPUA = 0x00;
    WPUB = 0x00;
    INTCON = 0x00; //未使用
    ADCON0 = 0x00;
    ADCON1 = 0x50; //ADCON設定 FOSC/16
    CCP1CON = 0x00; //使いません1（使えばよかった1)
    CCP2CON = 0x00; //使いません2（使えばよかった2)
    CCP3CON = 0x00; //使いません3（使えばよかった3)
    CCP4CON = 0x00; //使いません4（使えばよかった4)


    PORTA = 0x00; //PORTA初期化
    PORTB = 0x00; //PORTB初期化
    GIE = 1; //全体割り込み許可
    T0IE = 1; //タイマー割り込み許可
}

//wait us指定
static void wait_us(unsigned char i) {
    unsigned char x;
    for (x = 0; x < i; x++) {
        NOP();
        NOP();
    }
    return;
}