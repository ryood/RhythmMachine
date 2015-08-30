/*
 * Sequencer_I2C_Slave.c
 *
 * Created: 2015/08/26 17:08:41
 *  Author: gizmo
 *
 * �N���b�N�F����8MHz
 * �f�o�C�X�FATmega88V
 * Fuse bit�FhFuse:DFh lFuse:E2h eFuse:01h
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
 * 2015.08.30 I2C�����荞�ݏ���
 *
 */ 

#define 	F_CPU 8000000UL  // 8MHz

#include 	<avr/io.h>
#include	<avr/interrupt.h>
#include	<util/delay.h>

// TWI ��Ԓl
#define SR_SLA_ACK  0xA8		//SLA_R ��M�`�F�b�N
#define	SR_DATA_ACK	0xC0		//���M�p�P�b�g�`�F�b�N
#define	SR_ENDP_ACK	0xA0		//�I��or���s�[�g�`�F�b�N

// Shift Register
#define SHIFT_PORT PORTB
#define SHIFT_DATA PB4
#define SHIFT_RCK PB5

#define SHIFT_PORT_SCK PORTC
#define SHIFT_SCK PC0

volatile uint8_t sdata = 0x00;
volatile uint8_t prev_sdata = 0x00;

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
	TWAR=0xfe;
	
	// ���荞�݂�����
	// Enable TWI port, ACK, IRQ and clear interrupt flag
	TWCR = ((1<<TWEN) | (1<<TWEA) | (1<<TWIE) | (1<<TWINT));
}

ISR (TWI_vect)
{
	// ���荞�݂��Ƃ�LED��_�Łi�f�o�b�O�p�j
	PORTD ^= 0b10000000;
	
	switch (TWSR & 0xF8) {
	case SR_SLA_ACK:
		TWDR = sdata;
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

//�V�t�g���W�X�^�N���b�N������M
void _shift_sck()
{	
	SHIFT_PORT_SCK |= (1<<SHIFT_SCK);
	SHIFT_PORT_SCK &= ~(1<<SHIFT_SCK);
}

//���b�`�N���b�N������M
void _shift_rck()
{	
	SHIFT_PORT &= ~(1<<SHIFT_RCK);
	SHIFT_PORT |= (1<<SHIFT_RCK);
}

//�V���A���f�[�^��SER�ɏo��
void _shift_data(uint8_t bit)
{
	if (bit) {
		SHIFT_PORT |= (1<<SHIFT_DATA);
	} else {
		SHIFT_PORT &= ~(1<<SHIFT_DATA);
	}
}

void shift_out(uint8_t data){
	int8_t i;
	for (i=7; i>=0; i--) {   //��ʃr�b�g����W���M
		_shift_data((data>>i)&1);
		_shift_sck();
	}
	_shift_rck();   //���b�`���X�V
}

//------------------------------------------------//
// Main routine
//
//------------------------------------------------//
int main(){
	uint8_t rdata;
	uint8_t prev_rdata = 0x00;
	
	DDRB = 0x00;
	DDRC = 0x00;
	DDRD = 0x00;

	//sw input / pull up
	PORTD = 0b00111111;
	PORTB = 0b11000000;
	
	//Shift Register: SER, SCK, RCK output
	DDRB |= 0b00110000;
	DDRC |= 0b00000001;
	
	// LED Check
	PORTD |= 0b11000000;
	for (int i = 0; i <= 8; i++) {
		shift_out(0xFF >> i);
		_delay_ms(100);
	}
	PORTD &= 0b10111111;
	_delay_ms(100);
	PORTD &= 0b01111111;
	
	//�ʐM���W���[��������
	twi_init();
	
	sei();
	
	for(;;) {
		// sw�̉���������Ԃ�ǂݎ��
		do {
			rdata  = (~PIND & 0b00011111);
			rdata |= ((~PINB & 0b11000000) >> 1);
			rdata |= ((~PIND & 0b00100000) << 2);
		} while (rdata == prev_rdata);
		
		prev_rdata = rdata;
		
		// toggle
		sdata = prev_sdata ^ rdata;
		
		// �f�[�^��Shift Register�ɑ��M����B
		shift_out(sdata);
		
		// �f�[�^��TWI���M����B
		//twi_send(sdata);
		
		prev_sdata = sdata;
	}
}
