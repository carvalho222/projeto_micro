#include<P18F4550.h>
#include<delays.h>
#include "xlcd_picsim.c"
#include<timers.h>
#include <stdlib.h>
#include <usart.h>

// Configura��es
#pragma config CPUDIV = OSC1_PLL2
#pragma config	FOSC = HS			// Fosc = 20MHz -> Tcy = 200ns
#pragma config	WDT = OFF	// Watchdog desativado
#pragma config WDTPS = 512	// Postscaler WDT = 512	
#pragma config	PBADEN = OFF		// Pinos do PORTB come�am como digitais
#pragma config	LVP = OFF			// Desabilita grava��o em baixa tens�o

// Bot�es
#define BT0 PORTBbits.RB0
#define BT1 PORTBbits.RB1
#define BT2 PORTBbits.RB2

// Var�iveis Globais
char inc = 1;  
char segundos = 0;
char minutos = 0;
char horas = 0;
char menu = 0;
char confirma_acionado = 1;
char horario[8];
char sel;
char i = 0;
char dc, dc_str[3];
// Prot�tipos das fun��es
void ajusta_dc(unsigned int ciclo_ativo);
unsigned int ciclo_ativo = 0;

void main(void){
	
	TRISC = 0b00111111;
	//RC1 deve ser sa�da do PWM
	TRISB = 0b00000111;
	TRISE = 0x00;
	TRISD = 0x00;
	ADCON1 = 0x0F; 		// Todas as portas como digitais
	INTCON2bits.NOT_RBPU = 0;	// Habilita pull-ups da PORTB

// Configura��o das interrup��es
	RCONbits.IPEN = 1;		// habilita prioridade
	INTCONbits.GIEL = 1;	
	INTCONbits.GIEH = 1;

// Configura��o de INT0
	INTCONbits.INT0IE = 1;	// habilita INT0
	INTCONbits.INT0IF = 0;	// zera flag da INT0
	// INT0 � sempre de alta prioridade	

// Configura��o de INT1
	INTCON3bits.INT1IE = 1;	// habilita INT1
	INTCON3bits.INT1IF = 0;	// zera flag da INT1
	INTCON3bits.INT1IP = 1;	// interrup��o INT1 � de alta prioridade

// Configura��o de INT2
	INTCON3bits.INT2IE = 1;	// habilita INT2
	INTCON3bits.INT2IF = 0;	// zera flag da INT2
	INTCON3bits.INT2IP = 1;	// interrup��o INT2 � de alta prioridade

// Configura��o do TMR0
	T0CON = 0b10000110;
	INTCON2bits.TMR0IP = 1;	// interrup��o TMR0 � de alta prioridade
	INTCONbits.TMR0IF = 0;
	INTCONbits.TMR0IE = 1;
	WriteTimer0(26474);			// CARGA INICIAL = 26474 = 0X676A

// Configura��o do Timer 1
	IPR1bits.TMR1IP = 0; // TMR1 � de baixa prioridade
	PIR1bits.TMR1IF = 0;			
	WriteTimer1(64117);			// CARGA INICIAL = 64117
	T1CON = 0b11110100;	
	T1CONbits.T1SYNC = 0;

// Configura��o do Timer 2
	T2CON = 0b00000101; //TMR2 ON, prescaler 4, postscaler n importa, CI = 0
	PR2 = 249;
	// T = 200n*250*4 = 200us => f = 5000Hz.
	// (PR2 + 1) * 4 = 1000, maximo que ele chega � 1000, entao n sera utilizado 10 bits
	// de resolu��o, sera um pouco menos, logo a partir de 1000 ate 1023 sempre sera 100%
	// de ciclo ativo. Ent�o sempre que tiver em 1000 ser� 100% de duty cycle.

// Configura��o CCP2 - PWM
	CCP2CON = 0b00001100; //bits 4 e 5 s�o os dois bits menos significativos do duty cycle
	// os 8 mais significativos est�o em CCPR1L.
	
// Configura��es Serial
	TXSTAbits.BRGH = 1;
	TXSTAbits.SYNC = 0; 	// Assincrono
	BAUDCONbits.BRG16 = 0; 	// 8bits
	RCSTAbits.SPEN = 1; 	// Habilita o serial para o uso 
	TRISCbits.TRISC6 = 0; 	// Pino RC6 como sa�da
	TXSTAbits.TXEN = 1; 	// Habilita a transmissao
	SPBRG = 129;
	SPBRGH = 0;

// Interrup��es do Serial
/*PIR1bits.TXIF = 0;
PIE1bits.TXIE = 1;
IPR1bits.TXIP = 0;

PIE1bits.RCIE = 1;
PIR1bits.RCIF = 0;
IPR1bits.RCIP = 0;*/

//Configura��es CCP1
CCP1CON = 0b00001011; 
PIE1bits.CCP1IE = 1;
IPR1bits.CCP1IP = 0;
PIR1bits.CCP1IF = 0;
CCPR1H = 0x05;
CCPR1L = 0x8B; 

// Inicia��o do LCD
  	OpenXLCD(FOUR_BIT & LINES_5X7);	        // Comunica��o por byte e caracter 5x7 
  	WriteCmdXLCD(0x01);					 // comando para limpar a tela
  	Delay10KTCYx(8);			         // Delay necess�rio para a inicializa��o do LCD
                                         // 10000*200n*8 = 16ms
	WriteCmdXLCD(0x0C);				 // Comando para desligar cursor

	while(1) {
		
		PORTAbits.RA3 = 1;
		PORTAbits.RA5 = 0;
		
		switch(menu){
		case 0:
		WriteCmdXLCD(0xC0);
		putrsXLCD(" AJUSTE HORARIO ");
		T0CONbits.TMR0ON = 1;
		while(confirma_acionado == -1){
			T0CONbits.TMR0ON = 0;
			PORTAbits.RA3 = 0;
			PORTAbits.RA5 = 1;
			WriteCmdXLCD(0x84);		
			horario[6] = (0x30+(segundos/10));
			horario[7] = (0x30+(segundos%10));
			horario[5] = ':';
			horario[3] = (0x30 + (minutos/10));
			horario[4] = (0x30 + (minutos%10));
			horario[2] = ':';		
			horario[0] = (0x30 +(horas/10));
			horario[1] = (0x30 + (horas%10));
			putsXLCD(horario);
			WriteCmdXLCD(0xC0);
			putrsXLCD("  CONFIGURANDO   ");
		}
		break;
		case 1:
		WriteCmdXLCD(0xC0);
		putrsXLCD("     ALARME    ");
		while(confirma_acionado == -1){
		WriteCmdXLCD(0xC0);
		putrsXLCD("  TESTESSSS   ");
		}
		break;
		case 2:
		WriteCmdXLCD(0xC0);
		putrsXLCD("  ENVIAR SERIAL ");
		break;
		case 10:
		WriteCmdXLCD(0xC0);
		putrsXLCD(" ENVIANDO DADOS ");
		for(i = 0; i < 8; i++){
				while(TXSTAbits.TRMT == 0){};
				TXREG = horario[i];
			}
		while(TXSTAbits.TRMT == 0){};
		TXREG = '\n';
		while(TXSTAbits.TRMT == 0){};
		TXREG = '\r';
		break;
		case 3:
		WriteCmdXLCD(0xC0);
		putrsXLCD("    LAMPADA    ");
		while(confirma_acionado == -1){
		TRISC = 0b00111101;
		ajusta_dc(ciclo_ativo);
		dc = ciclo_ativo / 10;		// DC em %
		ltoa((long)dc, dc_str);		// Convers�o para ASCII
		WriteCmdXLCD(0xC0);
		putrsXLCD("    DC: ");
		putsXLCD(dc_str);
		putrsXLCD("%       ");
		Delay10KTCYx(50);
		}
		break;
		}

		WriteCmdXLCD(0x84);		
		horario[6] = (0x30+(segundos/10));
		horario[7] = (0x30+(segundos%10));
		horario[5] = ':';
		horario[3] = (0x30 + (minutos/10));
		horario[4] = (0x30 + (minutos%10));
		horario[2] = ':';		
		horario[0] = (0x30 +(horas/10));
		horario[1] = (0x30 + (horas%10));
		putsXLCD(horario);

	}

}

void ajusta_dc(unsigned int valor_dc){
	CCPR2L = (char)(valor_dc >> 2); 	// Retira os 2 bits menos significativos e converte para char 
	CCP2CONbits.DC2B0 = valor_dc%2; 	// Utiliza o LSB
	valor_dc = valor_dc >> 1; 			//Retira o LSB
	CCP2CONbits.DC2B1 = valor_dc%2;
}

//valor_dc � a variavel que contem os 10 bits de resolu��o que indicam qual o valor 
// no momento do meu duty cycle, � retirado os 2 bits LS deste valor e armazenado em CCPR2L
 