/****************************************************************************

    i2c.c - ���������� ������� I2C
            ��� �������� ������ SDK-1.1

    (C) i2c.c, ������ �.�.  2007 �.

��� ��������� ���������; �� ������ �������� �������������� �� �/���
�������������� �� � ������������ � ������������� ������������
��������� GNU, �������������� ������ ���������� ��; ���� ������ 2,
���� (�� ������ ������) ����� ����� ������� ������.

��� ��������� ���������������� � �������, ��� ��� ����� ��������,
�� ��� �����-���� ��������; ���� ��� ��������������� ��������
������������ �������� ��� ����������� ��� ���������� ����.  ���
��������� ��������� �������� �������� ������������� ������������
�������� GNU.

�� ������ ���� �������� ����� ������������� ������������ ��������
GNU ������ � ���� ����������; ���� ���, �������� �� ������: Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA

----------------------------------------------------------------------------
������, �����-���������, ������� �������������� ������� ��������� 
e-mail: kluchev@d1.ifmo.ru

****************************************************************************/

#include "aduc812.h"
#include "i2c.h"

/*----------------------------------------------------------------------------
                    �������
 -----------------------------------------------------------------------------*/

/**----------------------------------------------------------------------------
                        delay()
-------------------------------------------------------------------------------
�������� �� ����� ������� 25 ���.

����:       ���
�����:      ���
���������:  ���
----------------------------------------------------------------------------- */
static void delay( void )
{
char ch = 0;

    while( ch++ < 2 );
}

/**----------------------------------------------------------------------------
                        send_byte()
-------------------------------------------------------------------------------
������� �� I2C ����� (8 ���) � �������� ������������� (acknowledge).

����:       uchar ch - ����, ������� ����� �������.
�����:      ���
���������:  0 - ������������� (acknowledge) ��������
            1 - �� ���� �������������
----------------------------------------------------------------------------- */
static __bit send_byte( unsigned char ch ) //Returns ack (0 = acknowledged)
{
char i;
__bit ack;

    MDE = 1;//Output
    MCO = 0;

    for( i = 0 ; i < 8; i++, ch <<= 1 )
    {
        MDO = ( ch & 0x80 ) ? 1 : 0 ;
        MCO = 1;

        delay();

        MCO = 0;
    }

    MDE = 0;//Input
    MCO = 1;

    delay();

    ack = MDI;
    MCO = 0;
    return ack;
}

/**----------------------------------------------------------------------------
                        start()
-------------------------------------------------------------------------------
��������� ��������� Start ���������� i2c.

����:       ���
�����:      ���
���������:  ���
----------------------------------------------------------------------------- */
static void start(void)
{
    MDE = 1;//Output
    MDO = 1;
    MCO = 1;

    delay();

    MDO = 0;

    delay();

    MCO = 0;
}

/**----------------------------------------------------------------------------
                        stop()
-------------------------------------------------------------------------------
��������� ��������� Stop ���������� i2c.

����:       ���
�����:      ���
���������:  ���
----------------------------------------------------------------------------- */
static void stop(void)
{
    MDE = 1; //Output
    MCO = 0;
    MDO = 0;
    MCO = 1;

    delay();

    MDO = 1;

    delay();

    MDE = 0; //Input (release line)
}


/**----------------------------------------------------------------------------
                        begin()
-------------------------------------------------------------------------------
������ ������ �� ���������� i2c (Start + ������� i2c-������ slave-����������).

����:       uchar addr - i2c-����� slave-���������� 
�����:      ���
���������:  0 - ���������� ������������ (�������� acknowledge)
            1 - ���������� �� ������������
----------------------------------------------------------------------------- */

static __bit begin( unsigned char addr )//Returns ack (0 = acknowledged)
{
    start();

    return send_byte( addr );
}

/**----------------------------------------------------------------------------
                        Ack()
-------------------------------------------------------------------------------
������� ������������� (acknowledge) ����������.

����:       ���
�����:      ���
���������:  ���
----------------------------------------------------------------------------- */
static void ack( void )//Sends ack
{
    MDE = 1; //Output
    MCO = 0;
    MDO = 0;
    MCO = 1;

    delay();

    MCO = 0;
}

/**----------------------------------------------------------------------------
                        Nack()
-------------------------------------------------------------------------------
������� "���������������" (not acknowledged) ����������.

����:       ���
�����:      ���
���������:  ���
----------------------------------------------------------------------------- */
static void nack(void) //Sends NAck
{
    MDE = 1;
    MCO = 0;
    MDO = 1;
    MCO = 1;

    delay();

    MCO = 0;
}

/**----------------------------------------------------------------------------
                        get_ack()
-------------------------------------------------------------------------------
�������� �� ���������� slave-���������� � ������ (������ + ��������� ������� +
���������� ������)

����:       ���
�����:      ���
���������:  ���
----------------------------------------------------------------------------- */

__bit get_ack( unsigned char address ) //Returns 1 if there was an ACK
{
    I2CM = 1; //I2C Master mode

    if( begin( address & 0xFE ) ) 
    { 
        stop(); 
        return 0;
    }

    stop();

    return 1;
}


/**----------------------------------------------------------------------------
                        recv_byte()
-------------------------------------------------------------------------------
��������� 8 ��� � ���� ������ i2c ��� ������������� ��� ��������������� ������.

����:       ���
�����:      ���
���������:  �������� 8 ���.
----------------------------------------------------------------------------- */
unsigned char recv_byte(void)
{
char i;
unsigned char ch = 0;

    MDE = 0; //Input
    MCO = 0;

    for( i = 0; i < 8; i++ )
    {
        ch  <<= 1;
        MCO   = 1;

        delay();

        ch   |= MDI;

        MCO   = 0;
    }

    return ch;
}

/**----------------------------------------------------------------------------
                        receive_block()
-------------------------------------------------------------------------------
��������� ����� ������ �� i2c-���������� � 8-��������� ���������� �������� 
�������������. �������� ���� ���������� � ������� ������ __xdata.

����:       uchar address - I2C-����� ����������;
            uchar addr - ����� �� ���������� �������� ������������ ����������;
            uchar *block - ����� � ������� __xdata, ���� ����� ������� ��������
                ���� ������;
            uchar len - ����� ������������ �����.

�����:      ���
���������:  0 - �������;
            1 - ���������� �� �����������.
----------------------------------------------------------------------------- */
__bit receive_block(unsigned char address, unsigned char addr, unsigned char __xdata * block,unsigned char len)
{ //addr - address in target
unsigned char i, ch;

    I2CM    = 1;                    // I2C Master mode
    address = ( address & 0xFE );   // Write

    if( begin( address ) ) 
    {
        stop();
        return 1;
    }   //Error - No ACK

    if( send_byte( addr ) )
    {
        stop();
        return 1;
    }

    delay();
    delay();

    address |= 1;  //Read

    if( begin( address ) ) 
    {
        stop();
        return 1;
    }

    delay();

    if( len-1 )
    {
        for( i = 0; i< ( len - 1 ) ;i++)
        {
            ch = recv_byte();
            ack();

            *block++ = ch;
        }
    }

    ch = recv_byte();
    nack();

    *block = ch;
    stop();

    return 0;
}

/**----------------------------------------------------------------------------
                        send_block()
-------------------------------------------------------------------------------
������ ����� ������ �� ���������� ������ i2c-���������� � 8-��������� �������� 
�������������. 

����:       uchar address - I2C-����� ����������;
            uchar addr - ����� �� ���������� �������� ������������ ����������,
                ���� ����� ������� ����;
            uchar *block - ����� � ������� __xdata, ��� ������������� ������ ��� 
                ���������;
            uchar len - ����� ������������� �����.

�����:      ���
���������:  0 - �������;
            1 - ���������� �� �����������.
----------------------------------------------------------------------------- */
__bit transmit_block(unsigned char address, unsigned char addr, unsigned char __xdata * block,unsigned char len)
{ //addr - address in target
unsigned char ch, i;

    I2CM    = 1;                //I2C Master mode
    address = address & 0xFE;   //Write

    if( begin( address ) ) 
    {
        stop();
        return 1;
    }   //Error - no Ack

    if( send_byte( addr ) ) 
    {
        stop();
        return 1;
    }

    for( i = 0; i < len; i++,block++)
    {
        ch = *block;
        if( send_byte( ch ) )
        { 
            stop(); 
            return 1;
        }   //Not to the end of the block
    }

    stop();

    return 0;
}

/**----------------------------------------------------------------------------
                        receive_block16()
-------------------------------------------------------------------------------
��������� ����� ������ �� i2c-���������� � 16-��������� ���������� �������� 
�������������. �������� ���� ���������� � ������� ������ __xdata.

����:       uchar address - I2C-����� ����������;
            ushort addr - ����� �� ���������� �������� ������������ ����������;
            uchar *block - ����� � ������� __xdata, ���� ����� ������� ��������
                ���� ������;
            ushort len - ����� ������������ �����.

�����:      ���
���������:  0 - �������;
            1 - ���������� �� �����������.
----------------------------------------------------------------------------- */
__bit receive_block16(unsigned char address, unsigned short addr, unsigned char __xdata * block,unsigned short len)
{ //addr - address in target
unsigned char ch;
unsigned short i;

    I2CM     = 1;       // I2C Master mode
    address &= 0xFE;    // Write

    if( begin(address) )
    {
        stop();     
        return 1;
    } // Error - No ACK

    if( send_byte( ( unsigned char )( addr >> 8) ) )
    {
        stop(); 
        return 1;
    }

    if( send_byte( ( unsigned char )addr ) )
    {
        stop(); 
        return 1;
    }

    delay();
    delay();

    address |= 1; //Read

    if( begin( address ) ) 
    {
        stop();
        return 1;
    }

    delay();

    if( len - 1 )
    {
        for( i = 0; i < ( len - 1 ); ++i )
        {
            ch = recv_byte();
            ack();

            *block++ = ch;
        }
    }

    ch = recv_byte();

    nack();

    *block = ch;

    stop();

    return 0;
}

/**----------------------------------------------------------------------------
                        SendBlock16()
-------------------------------------------------------------------------------
������ ����� ������ �� ���������� ������ i2c-���������� � 16-��������� �������� 
�������������. 

����:       uchar address - I2C-����� ����������;
            ushort addr - ����� �� ���������� �������� ������������ ����������,
                ���� ����� ������� ����;
            uchar *block - ����� � ������� __xdata, ��� ������������� ������ ��� 
                ���������;
            ushort len - ����� ������������� �����.

�����:      ���
���������:  0 - �������;
            1 - ���������� �� �����������.
----------------------------------------------------------------------------- */
__bit transmit_block16(unsigned char address, unsigned short addr, unsigned char __xdata * block,unsigned short len)
{ //addr - address in target
unsigned char ch;
unsigned short i;

    I2CM     = 1;    // I2C Master mode
    address &= 0xFE; // Write

    if( begin( address ) ) 
    {
        stop(); 
        return 1;

    } // Error - no Ack

    if( send_byte( ( unsigned char )( addr >> 8) ) ) 
    {
        stop(); 
        return 1;
    }

    if( send_byte( ( unsigned char )addr ) ) 
    {
        stop(); 
        return 1;
    }

    for( i = 0; i < len; i++, block++ )
    {
        ch = *block;

        if( send_byte( ch ) ) 
        {
            stop(); 
            return 1;
        } // Not to the end of the block
    }

    stop();

    return 0;
}
