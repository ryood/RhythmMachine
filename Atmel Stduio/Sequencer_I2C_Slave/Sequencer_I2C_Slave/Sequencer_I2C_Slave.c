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
 * PORTC[PC3]     : LED(�f�o�b�O�p)
 * 
 * AtmelStudio 6.2
 *
 * 2015.09.01 TWI�G���[���ɃX�e�[�^�X�R�[�h��LED�ɕ\��
 * 2015.09.01 �f�o�b�O�p��PC3��LED��ڑ�
 * 2015.09.01 �V�[�P���X��z��ɕύX
 * 2015.08.30 Pin Change Interrupt�ƃ`���^�����O�΍�
 * 2015.08.30 I2C�����荞�ݏ���
 *
 */ 

#define 	F_CPU 8000000UL  // 8MHz

#include 	<avr/io.h>
#include	<avr/interrupt.h>
#include	<util/delay.h>

// TWI �X���[�u�E�A�h���X
#define TWI_SLAVE_ADDRESS 0xFE

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

// ���ϐ�
// �V�[�P���X�E�X�C�b�`
volatile uint8_t sequence_data[2];		// �V�[�P���X�E�X�C�b�`�̃g�O�����
volatile uint8_t sequence_n;			// �V�[�P���X�̕\�� 
volatile uint8_t sequence_rd;			// �V�[�P���X�E�X�C�b�`�̓ǂݎ��l
volatile uint8_t sequence_n_rd;

// Potentiometer
volatile uint8_t pot_data[2];

// Rotary Encoder
volatile uint8_t re_data;
volatile uint8_t re_sw;


void shift_out(uint8_t data);

//------------------------------------------------//
// TWI
//
//------------------------------------------------//
void twi_error()
{
	uint8_t twi_status = TWSR & 0xF8;
	
	shift_out(twi_status);	
	
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
	
	// ���荞�݂�����
	// Enable TWI port, ACK, IRQ and clear interrupt flag
	TWCR = ((1<<TWEN) | (1<<TWEA) | (1<<TWIE) | (1<<TWINT));
}

ISR (TWI_vect)
{
	// ���荞�݂��Ƃ�LED��_�Łi�f�o�b�O�p�j
	PORTC ^= (1 << PC3);
	
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

void shift_out(uint8_t data)
{
	int8_t i;
	for (i=7; i>=0; i--) {   //��ʃr�b�g����W���M
		_shift_data((data>>i)&1);
		_shift_sck();
	}
	_shift_rck();   //���b�`���X�V
}

//------------------------------------------------//
// Switch�̏���
//
//------------------------------------------------//

void init_switches()
{
	// Pin Change Interrupt�̗L����
	PCICR = (1 << PCIE0) | (1 << PCIE2);
	PCMSK0 = 0b11000001;	// PORTB
	PCMSK2 = 0b00111111;	// PORTD
	
	// TIMER0 �I�[�o�[�t���[���荞�݂̗L����
	TCCR0B = 0x00;	// Timer0��~
	TIMSK0 = (1 << TOIE0);
}

// �V�[�P���X�E�X�C�b�`�̉���������Ԃ�ǂݎ��
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
	
	// Timer0���~
	TCCR0B = 0x00;
	
	// �V�[�P���X�\���؂�ւ��X�C�b�`���g�O������œǂݎ��
	// PINx���W�X�^�̒l�͂�������ϐ��ɑ�����Ȃ��Ɣ�r�����܂������Ȃ�
	tmp = (~PINB & (1 << PB0));
	if (sequence_n_rd == tmp) {
		sequence_n ^= sequence_n_rd;
		
		// �g�O����Ԃ�LED�ɕ\��
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
		
		// �g�O����Ԃ�LED�ɕ\��
		shift_out(sequence_data[sequence_n]);
	}
		
	// Pin Change Interrupt�̗L����
	PCICR = (1 << PCIE0) | (1 << PCIE2);
}

void pin_change_interrupt_handler()
{
	// Pin Change Interrupt�𖳌���
	PCICR = 0x00;
	
	// ���荞�݂��Ƃ�LED��_�Łi�f�o�b�O�p�j
	PORTC ^= (1 << PC3);
		
	sequence_rd = read_sequence_switches();
	sequence_n_rd = (~PINB & (1 << PB0));
	
	// Timer0���N��
	TCCR0B = 0x05;	// �v���X�P�[���F1024
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

	// Switch input / pull up
	PORTD = 0b00111111;
	PORTB = 0b11000001;
	
	// Shift Register: SER, SCK, RCK output
	DDRB |= (1 << SHIFT_DATA) | (1 << SHIFT_RCK);
	DDRC |= (1 << SHIFT_SCK);
	
	// LED
	DDRD |= (1 << PD7) | (1 << PD6);
	DDRC |= (1 << PC3);		// �f�o�b�O�p
	
	// LED Check
	PORTD |= (1 << PD7) | (1 << PD6);
	PORTC |= (1 << PC3);	// �f�o�b�O�p
	for (int i = 0; i <= 8; i++) {
		shift_out(0xFF >> i);
		_delay_ms(100);
	}
	PORTD &= ~(1 << PD6);
	_delay_ms(100);
	PORTD &= ~(1 << PD7);
	_delay_ms(100);
	PORTC &= ~(1 << PC3);	// �f�o�b�O�p
		
	init_switches();
	twi_init();
		
	sei();
	
	for(;;) {
		// nop
	}
}
