/*
 * Sequencer_I2C_Slave.c
 *
 * Created: 2015/08/26 17:08:41
 *  Author: gizmo
 *
 * クロック：内蔵8MHz
 * デバイス：ATmega88V
 * Fuse bit：hFuse:DFh lFuse:E2h eFuse:01h
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
 * PORTB[PB1]     : Rotary Encoder SW
 * PORTB[PB2]     : Rotary Encoder A
 * PORTB[PB3]     : Rotary Encoder B
 *
 * PORTB[PB4]     : 74HC595 SER
 * PORTB[PB5]     : 74HC595 RCK
 * PORTC[PC0]     : 74HC595 SCK
 * 
 * AtmelStudio 6.2
 *
 * 2015.08.30 Pin Change Interruptとチャタリング対策
 * 2015.08.30 I2Cを割り込み処理
 *
 */ 

#define 	F_CPU 8000000UL  // 8MHz

#include 	<avr/io.h>
#include	<avr/interrupt.h>
#include	<util/delay.h>

// TWI スレーブ・アドレス
#define TWI_SLAVE_ADDRESS 0xFE

// TWI 状態値
#define SR_SLA_ACK  0xA8		//SLA_R 受信チェック
#define	SR_DATA_ACK	0xC0		//送信パケットチェック
#define	SR_ENDP_ACK	0xA0		//終了orリピートチェック

// Shift Register
#define SHIFT_PORT PORTB
#define SHIFT_DATA PB4
#define SHIFT_RCK PB5

#define SHIFT_PORT_SCK PORTC
#define SHIFT_SCK PC0

// 大域変数
// シーケンス・スイッチ
volatile uint8_t sequence_data[2];		// シーケンス・スイッチのトグル状態
volatile uint8_t sequence_n;			// シーケンスの表裏 
volatile uint8_t sequence_rd;			// シーケンス・スイッチの読み取り値
volatile uint8_t sequence_n_rd;

// Potentiometer
volatile uint8_t pot_data[2];

// Rotary Encoder
volatile uint8_t re_data;
volatile uint8_t re_sw;


//------------------------------------------------//
// TWI
//
//------------------------------------------------//
void twi_error()
{
	while(1) {
		PORTD = 0b10000000;
		_delay_ms(100);
		PORTD = 0b01000000;
		_delay_ms(100);
	}
}

void twi_init()
{
	// 8MHz clk, bit rate 100kHz
	TWBR = 2;
	TWSR = 0x02;
	
	//slave address
	TWAR = TWI_SLAVE_ADDRESS;
	
	// 割り込みを許可
	// Enable TWI port, ACK, IRQ and clear interrupt flag
	TWCR = ((1<<TWEN) | (1<<TWEA) | (1<<TWIE) | (1<<TWINT));
}

ISR (TWI_vect)
{
	// 割り込みごとにLEDを点滅（デバッグ用）
	PORTD ^= 0b10000000;
	
	switch (TWSR & 0xF8) {
	case SR_SLA_ACK:
		TWDR = sequence_data[sequence_n];
		break;
	case SR_DATA_ACK:
		break;
	default:
		twi_error();
	}
	
	TWCR |= (1<<TWINT);	// Clear TWI interrupt flag
}

//------------------------------------------------//
// Shift Register (74HC595)
//
//------------------------------------------------//

//シフトレジスタクロックを一つ送信
void _shift_sck()
{	
	SHIFT_PORT_SCK |= (1<<SHIFT_SCK);
	SHIFT_PORT_SCK &= ~(1<<SHIFT_SCK);
}

//ラッチクロックを一つ送信
void _shift_rck()
{	
	SHIFT_PORT &= ~(1<<SHIFT_RCK);
	SHIFT_PORT |= (1<<SHIFT_RCK);
}

//シリアルデータをSERに出力
void _shift_data(uint8_t bit)
{
	if (bit) {
		SHIFT_PORT |= (1<<SHIFT_DATA);
	} else {
		SHIFT_PORT &= ~(1<<SHIFT_DATA);
	}
}

void shift_out(uint8_t data)
{
	int8_t i;
	for (i=7; i>=0; i--) {   //上位ビットから８個送信
		_shift_data((data>>i)&1);
		_shift_sck();
	}
	_shift_rck();   //ラッチを更新
}

//------------------------------------------------//
// Switchの処理
//
//------------------------------------------------//

void init_switches()
{
	// Pin Change Interruptの有効化
	PCICR = (1 << PCIE0) | (1 << PCIE2);
	PCMSK0 = 0b11000001;	// PORTB
	PCMSK2 = 0b00111111;	// PORTD
	
	// TIMER0 オーバーフロー割り込みの有効化
	TCCR0B = 0x00;	// Timer0停止
	TIMSK0 = (1 << TOIE0);
}

// シーケンス・スイッチの押し下げ状態を読み取り
uint8_t read_sequence_switches()
{
	uint8_t data;
	
	data  = (~PIND & 0b00011111);
	data |= ((~PINB & 0b11000000) >> 1);
	data |= ((~PIND & 0b00100000) << 2);
	
	return data;
}

ISR (TIMER0_OVF_vect)
{
	uint8_t tmp;
	
	// Timer0を停止
	TCCR0B = 0x00;
	
	// シーケンス表裏切り替えスイッチをトグル動作で読み取り
	// PINxレジスタの値はいったん変数に代入しないと比較がうまくいかない
	tmp = (~PINB & (1 << PB0));
	if (sequence_n_rd == tmp) {
		sequence_n ^= sequence_n_rd;
		
		// トグル状態をLEDに表示
		if (sequence_n) {
			PORTD &= ~(1 << PD6);
			PORTD |= (1 << PD7);
		} else {
			PORTD &= ~(1 << PD7);
			PORTD |= (1 << PD6);
		}		
	}
	
	tmp = read_sequence_switches();
	if (sequence_rd == tmp) {
		sequence_data[sequence_n] ^= sequence_rd;
		
		// トグル状態をLEDに表示
		shift_out(sequence_data[sequence_n]);
	}
		
	// Pin Change Interruptの有効化
	PCICR = (1 << PCIE0) | (1 << PCIE2);
}

void pin_change_interrupt_handler()
{
	// Pin Change Interruptを無効化
	PCICR = 0x00;
	
	// 割り込みごとにLEDを点滅（デバッグ用）
	PORTD ^= (1 << PD6);
		
	sequence_rd = read_sequence_switches();
	sequence_n_rd = (~PINB & (1 << PB0));
	
	// Timer0を起動
	TCCR0B = 0x05;	// プリスケーラ：1024
	TCNT0 = 80;		// about: 10ms	
}

ISR (PCINT0_vect)
{
	pin_change_interrupt_handler();
}

ISR (PCINT2_vect)
{
	pin_change_interrupt_handler();
}

//------------------------------------------------//
// Main routine
//
//------------------------------------------------//
int main()
{
	DDRB = 0x00;
	DDRC = 0x00;
	DDRD = 0x00;

	//sw input / pull up
	PORTD = 0b00111111;
	PORTB = 0b11000001;
	
	//Shift Register: SER, SCK, RCK output
	DDRB |= (1 << SHIFT_DATA) | (1 << SHIFT_RCK);
	DDRC |= (1 << SHIFT_SCK);	
	
	// LED Check
	PORTD |= 0b11000000;
	for (int i = 0; i <= 8; i++) {
		shift_out(0xFF >> i);
		_delay_ms(100);
	}
	PORTD &= 0b10111111;
	_delay_ms(100);
	PORTD &= 0b01111111;
	
	init_switches();
	twi_init();
		
	sei();
	
	for(;;) {
		// 
	}
}
