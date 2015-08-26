/*
 * Sequencer_I2C_Slave.c
 *
 * Created: 2015/08/26 17:08:41
 *  Author: gizmo
 *
 * クロック：内蔵1MHz
 * デバイス：ATmega88V
 * Fuse bit：hFuse:DFh lFuse:62h eFuse:01h
 *
 * PC4:SDA
 * PC5:SCK
 *
 * PortD[PD0..PD4]: SWx5
 * PortB[PB6..PB7]: SWx2
 * PortD[PD5]     : SWx1
 *
 * PortD[PD6..PD7]: LEDx2
 * PortD[PB0]     : SWx1
 *
 * AtmelStudio 6.2
 *
 */ 

#define 	F_CPU 1000000UL  // 1MHz

#include 	<avr/io.h>
#include	<util/delay.h>

//TWI 状態値
#define SR_SLA_ACK  0xA8		//SLA_R 受信チェック
#define	SR_DATA_ACK	0xC0		//送信パケットチェック
#define	SR_ENDP_ACK	0xA0		//終了orリピートチェック

//------------------------------------------------//
// TWI
//
//------------------------------------------------//
void twi_error(){
	PORTD = 0b11000000;
	while(1);
}

void twi_init(){
	//TWBR = 0xFF;	//分周	2KHz
	// 8MHz clk, bit rate 100kHz
	TWBR = 2;
	TWSR = 0x02;
	TWCR = 1<< TWEN;
	
	//slave address
	TWAR=0xfe;
}

//データパケットを１バイト送る。
void twi_send(uint8_t sdata){
	
	TWCR = (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
	
	//　Wait for TWINT Flag set.
	while( !( TWCR & (1<<TWINT) ) );
	
	if( (TWSR & 0xF8 ) != SR_SLA_ACK )
		twi_error();
		
	// データを送信
	TWDR = sdata;
	
	TWCR = (1<<TWINT) | (1<<TWEA) | (1<<TWEN );

	//　Wait for TWINT Flag set.
	while(  ! ( TWCR & ( 1<<TWINT ) ) );
	
	if((TWSR & 0xF8 ) != SR_DATA_ACK)
		twi_error();
}

//------------------------------------------------//
// Main routine
//
//------------------------------------------------//
int main(){
	uint8_t sdata;
	uint8_t prev_sdata = 0x00;


	//sw input / pull up
	DDRD = 0x00;
	PORTD = 0b00111111;
	
	DDRB = 0x00;
	PORTB = 0b11000000;
	
	//Error LED
	DDRB |= 0b11000010;

	//PORTB = 0b11000000;

	//通信モジュール初期化
	twi_init();
	
	for(;;) {
		// swの押し下げ状態を読み取る
		sdata  = (~PIND & 0b00011111);
		sdata |= ((~PINB & 0b11000000) >> 1);
		sdata |= ((~PIND & 0b00100000) << 2);
		
		// toggle
		sdata = prev_sdata ^ sdata;
		
		//データを送信する。
		twi_send(sdata);
		
		prev_sdata = sdata;
	}
}
