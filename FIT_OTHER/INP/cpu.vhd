-- cpu.vhd: Simple 8-bit CPU (BrainFuck interpreter)
-- Copyright (C) 2015 Brno University of Technology,
--                    Faculty of Information Technology
-- Author(s): Adam Bezak xbezak01
--

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

-- ----------------------------------------------------------------------------
--                        Entity declaration
-- ----------------------------------------------------------------------------
entity cpu is
 port (
   CLK   : in std_logic;  -- hodinovy signal
   RESET : in std_logic;  -- asynchronni reset procesoru
   EN    : in std_logic;  -- povoleni cinnosti procesoru
 
   -- synchronni pamet RAM
   DATA_ADDR  : out std_logic_vector(12 downto 0); -- adresa do pameti
   DATA_WDATA : out std_logic_vector(7 downto 0); -- mem[DATA_ADDR] <- DATA_WDATA pokud DATA_EN='1'
   DATA_RDATA : in std_logic_vector(7 downto 0);  -- DATA_RDATA <- ram[DATA_ADDR] pokud DATA_EN='1'
   DATA_RDWR  : out std_logic;                    -- cteni (1) / zapis (0)
   DATA_EN    : out std_logic;                    -- povoleni cinnosti
   
   -- vstupni port
   IN_DATA   : in std_logic_vector(7 downto 0);   -- IN_DATA <- stav klavesnice pokud IN_VLD='1' a IN_REQ='1'
   IN_VLD    : in std_logic;                      -- data platna
   IN_REQ    : out std_logic;                     -- pozadavek na vstup data
   
   -- vystupni port
   OUT_DATA : out  std_logic_vector(7 downto 0);  -- zapisovana data
   OUT_BUSY : in std_logic;                       -- LCD je zaneprazdnen (1), nelze zapisovat
   OUT_WE   : out std_logic                       -- LCD <- OUT_DATA pokud OUT_WE='1' a OUT_BUSY='0'
 );
end cpu;


-- ----------------------------------------------------------------------------
--                      Architecture declaration
-- ----------------------------------------------------------------------------
architecture behavioral of cpu is
-- ----------------------------------------------------------------------------
--						Signaly
-- ----------------------------------------------------------------------------
signal PC_reg    : std_logic_vector(12 downto 0);
signal PC_inc    : std_logic;
signal PC_dec    : std_logic;

signal PTR_reg    : std_logic_vector(12 downto 0);
signal PTR_inc    : std_logic;
signal PTR_dec    : std_logic;

signal TMP_reg	: std_logic_vector(7 downto 0);
signal TMP_ld : std_logic;

signal CNT_reg 		: std_logic_vector(7 downto 0);
signal CNT_inc		: std_logic;
signal CNT_dec 		: std_logic;

signal inst_reg		: std_logic_vector(7 downto 0);
signal inst_ldata 	: std_logic;

signal sel1 : std_logic;
signal sel2 : std_logic_vector(1 downto 0); -- 00 01 10 11

-- ----------------------------------------------------------------------------
--						Konecny automat
-- ----------------------------------------------------------------------------
type fsm_state is (sidle, sfetch0, sfetch1, sdecode, -- inicializacia, dekodovanie instrukcie
	s_inc_ptr, s_dec_ptr, -- inkrementacia, dekrementacia ukazatela
	s_inc_b, s_dec_b, -- inkrementacia, dekrementacia hodnoty aktualnej bunky
	ss_inc_b, ss_dec_b, -- 2. faza po povoleni zapisu do RAM
	s_while1, s_while2, -- jednoduchy while cyklus
	s_while3, s_while4,
	se_while1, se_while2,
	se_while3, se_while4, se_while5,
	s_putchar, s_getchar, -- vytiskni, nacti hodnotu
	ss_putchar, ss_getchar, --2. faza
	s_save, s_save_reverse, -- uloz hodnotu do pomocnej promennej, reverse
	s_save_1, s_save_reverse1,
	s_halt, sskip -- return ;
	);

signal pstate : fsm_state;
signal nstate : fsm_state;

-- ----------------------------------------------------------------------------
--						Instrukcie
-- ----------------------------------------------------------------------------
type instruction_type is (
	inc_ptr, dec_ptr, inc_b, dec_b, whiles1, whilee1, putchar, getchar, save, save_reverse, halt, skip
	);

signal instruction : instruction_type;


begin
-- ----------------------------------------------------------------------------
--						Register PC
-- ----------------------------------------------------------------------------
PC_register: process(RESET, CLK)
begin
	if (RESET = '1') then
		PC_reg <= (others=>'0');
	elsif (CLK'event) and (CLK = '1') then
		if (PC_inc = '1') and (PC_dec = '0') then -- inkrementujem ukazatel do pamati programu
			PC_reg <= PC_reg + 1;
		elsif (PC_dec = '1') and (PC_inc = '0') then -- dekrementujem ukazatel do pamati programu
			PC_reg <= PC_reg - 1;
		end if;
	end if;	
end process;

-- ----------------------------------------------------------------------------
--						Register PTR
-- ----------------------------------------------------------------------------
PTR_register: process(RESET, CLK)
begin
	if (RESET = '1') then
		PTR_reg <= "1000000000000";					
	elsif (CLK'event) and (CLK = '1') then
		if (PTR_inc = '1') and (PTR_dec = '0') then
			PTR_reg(11 downto 0) <= PTR_reg(11 downto 0) + 1;
		elsif (PTR_dec = '1') and (PTR_inc = '0') then
			PTR_reg(11 downto 0) <= PTR_reg(11 downto 0) - 1;
		end if;
	end if;	
end process;

-- ----------------------------------------------------------------------------
--						Register TMP
-- ----------------------------------------------------------------------------
TMP_register: process(RESET, CLK)
begin
	if (RESET = '1') then
		TMP_reg <= (others => '0');
	elsif (CLK'event) and (CLK = '1') then
		if (TMP_ld = '1') then
			TMP_reg <= DATA_RDATA;
		end if;
	end if;	
end process;

-- ----------------------------------------------------------------------------
--						Register CNT
-- ----------------------------------------------------------------------------
CNT_register: process (RESET, CLK)
begin
	if (RESET = '1') then --asynchronny reset
		CNT_reg <= (others => '0');
	elsif (CLK'event) and (CLK = '1') then
		if (CNT_inc = '1') and (CNT_dec = '0') then
			CNT_reg <= CNT_reg + 1;
		elsif (CNT_dec = '1') and (CNT_inc = '0') then
			CNT_reg <= CNT_reg - 1; 
		end if;
	end if;
end process;

-- ----------------------------------------------------------------------------
--						Multiplexor 1
-- ----------------------------------------------------------------------------
MX1: process(CLK, PTR_reg, PC_reg, sel1)
begin
	case (sel1) is
		when '0' => DATA_ADDR <= PTR_reg;
		when '1' => DATA_ADDR <= PC_reg;
		when others =>
	end case;
end process;

-- ----------------------------------------------------------------------------
--						Multiplexor 2
-- ----------------------------------------------------------------------------
MX2: process(CLK, sel2, DATA_RDATA, IN_DATA, TMP_reg)
begin
	case (sel2) is
		when "00" => DATA_WDATA <= IN_DATA;
		when "01" => DATA_WDATA <= TMP_reg;
		when "10" => DATA_WDATA <= DATA_RDATA - 1;
		when "11" => DATA_WDATA <= DATA_RDATA + 1;
		when others =>
	end case;
end process;

-- ----------------------------------------------------------------------------
--						Register instrukcii IR
-- ----------------------------------------------------------------------------
instruction_reg: process (RESET, CLK)
begin
	if (RESET = '1') then --asynchronny reset
		inst_reg <= (others => '0');
	elsif (CLK'event) and (CLK = '1') then
		if (inst_ldata = '1') then
			inst_reg <= DATA_RDATA;
		end if;
	end if;	
end process;
-- ----------------------------------------------------------------------------
--						Dekoder instrukcii
-- ----------------------------------------------------------------------------
instruction_dec: process(inst_reg)
begin
	case (inst_reg) is
		when X"3E" => instruction <= inc_ptr;  --">"
		when X"3C" => instruction <= dec_ptr;  --"<"
		when X"2B" => instruction <= inc_b;  --"+"
		when X"2D" => instruction <= dec_b;  --"-"
		when X"5B" => instruction <= whiles1;  --"["
		when X"5D" => instruction <= whilee1;  --"]"
		when X"2E" => instruction <= putchar;  --"."
		when X"2C" => instruction <= getchar;  --","
		when X"24" => instruction <= save;  --"$"
		when X"21" => instruction <= save_reverse;  --"!"
		when X"00" => instruction <= halt;  --"null"
		when others => instruction <= skip; 
	end case;
end process;

-- ----------------------------------------------------------------------------
--						Konecny automat
-- ----------------------------------------------------------------------------
--aktualny stav
fsm_pstate: process (RESET, CLK)
	begin
		if (RESET = '1') then
			pstate <= sidle;
		elsif (CLK'event) and (CLK = '1') then
			if (EN = '1') then
				pstate <= nstate;
			end if;
		end if;
	end process;

--nasledujuci stav
fsm_nstate: process (pstate, instruction, IN_DATA, DATA_RDATA, IN_VLD, OUT_BUSY)
begin
	--RAM
	DATA_EN <= '0';
	DATA_RDWR <= '0';
	--PC
	PC_inc <= '0';
	PC_dec <= '0';
	--PTR
	PTR_dec <= '0';
	PTR_inc <= '0';
	--CNT
	CNT_inc <= '0';
	CNT_dec <= '0';
	--InputOutput
	IN_REQ <= '0';
	OUT_WE <= '0';
	OUT_DATA <= (others => '0');
	--TMP
	TMP_ld <= '0';
	sel1 <= '0';
	sel2 <= "00";
	inst_ldata <= '0';
	
	case (pstate) is
		--IDLE
		when sidle =>
			nstate <= sfetch0;

		--FETCH
		when sfetch0 =>
			nstate <= sfetch1;
			DATA_EN <= '1';
			DATA_RDWR <= '1';
			sel1 <= '1';
		when sfetch1 =>
			inst_ldata <= '1';
			nstate <= sdecode;
		--DECODE
		when sdecode =>
			case (instruction) is
				when halt 		=> nstate <= s_halt;
				when inc_ptr 	=> nstate <= s_inc_ptr;
				when dec_ptr 	=> nstate <= s_dec_ptr;
				when inc_b 		=> nstate <= s_inc_b;
				when dec_b 		=> nstate <= s_dec_b;
				when whiles1 	=> nstate <= s_while1;
				when whilee1 	=> nstate <= se_while1;
				when putchar 	=> nstate <= s_putchar;
				when getchar 	=> nstate <= s_getchar;
				when save 		=> nstate <= s_save;
				when save_reverse => nstate <= s_save_reverse;
				when others 	=> nstate <= sskip;
			end case;
		
		--INSTRUCTION
		-- skip
		when sskip =>
			PC_inc <= '1';
			PC_dec <= '0';
			nstate <= sfetch0;
		-- halt
		when s_halt =>
			nstate <= s_halt;
		-- '>'
		when s_inc_ptr =>
			PTR_inc <= '1';
			PTR_dec <= '0';
			PC_inc <= '1';
			PC_dec <= '0';
			nstate <= sfetch0;

		-- '<'
		when s_dec_ptr =>
			PTR_dec <= '1';
			PTR_inc <= '0';
			PC_inc <= '1';
			PC_dec <= '0';
			nstate <= sfetch0;

		-- '+'
		when s_inc_b =>
			DATA_EN <= '1'; -- povolenie praci s RAM
			DATA_RDWR <= '1'; -- read
			sel1 <= '0';
			nstate <= ss_inc_b;
		when ss_inc_b =>
			DATA_EN <= '1';
			DATA_RDWR <= '0'; -- write
			sel1 <= '0';
			sel2 <= "11"; -- RDATA + 1
			PC_inc <= '1';
			PC_dec <= '0';
			nstate <= sfetch0;

		-- '-'
		when s_dec_b =>
			DATA_EN <= '1';
			DATA_RDWR <= '1';
			sel1 <= '0';
			nstate <= ss_dec_b;
		when ss_dec_b =>
			DATA_EN <= '1';
			DATA_RDWR <= '0';
			sel1 <= '0';
			sel2 <= "10"; --RDATA - 1
			PC_inc <= '1';
			PC_dec <= '0';
			nstate <= sfetch0;
			
		-- '.'
		when s_putchar =>
			sel1 <= '0';
			DATA_EN <= '1';
			DATA_RDWR <= '1'; --read
			nstate <= ss_putchar;
		when ss_putchar =>
			if (OUT_BUSY = '0') then
				OUT_WE <= '1';
				OUT_DATA <= DATA_RDATA;
				PC_inc <= '1';
				PC_dec <= '0';
				nstate <= sfetch0;
			else nstate <= ss_putchar;
			end if;
		
		-- ','
		when s_getchar =>
			IN_REQ <= '1';
			nstate <= ss_getchar;
		when ss_getchar =>
			if (IN_VLD = '1') then
				IN_REQ <= '1';
				DATA_EN <= '1';
				DATA_RDWR <= '0';
				sel2 <= "00";
				sel1 <= '0';
				PC_inc <= '1';
				PC_dec <= '0';
				nstate <= sfetch0;
			else nstate <= ss_getchar;
			end if;
		-- '$'
		when s_save =>
			DATA_EN <= '1';
			DATA_RDWR <= '1';
			sel1 <= '0';
			nstate <= s_save_1;
		when s_save_1 =>
			TMP_ld <= '1';
			PC_inc <= '1';
			PC_dec <= '0';
			nstate <= sfetch0;

		-- '!'
		when s_save_reverse =>
			DATA_EN <= '1';
			DATA_RDWR <= '0';
			sel2 <= "11";
			sel1 <= '0';
			PC_inc <= '1';
			PC_dec <= '0';
			nstate <= sfetch0;
			
         -- while loops
        when s_while1 =>
            nstate <= s_while2;
            sel1 <= '0';
            DATA_EN <= '1';
            PC_inc <= '1';
			PC_dec <= '0';
			DATA_RDWR <= '1';

        when s_while2 =>
            nstate <= sfetch0;
            if (DATA_RDATA = "00000000") then 
                CNT_inc <= '1';
				CNT_dec <= '0';
				nstate <= s_while3;   
            end if;
         
        when s_while3 =>
            nstate <= s_while4; --start
            DATA_EN <= '1';
			DATA_RDWR <= '1';
            sel1 <= '1';

        when s_while4 =>
            nstate <= s_while3; -- znovu zaciatok 
            if (CNT_reg = "00000000") then
               nstate <= sfetch0;
            else
				if (DATA_RDATA = X"5B") then
					CNT_inc <= '1';
					CNT_dec <= '0';
				elsif (DATA_RDATA = X"5D") then
					CNT_dec <= '1';
					CNT_inc <= '0';
               end if;
			PC_inc <= '1';
			PC_dec <= '0';
            end if;  

         -- while end
        when se_while1 =>
            nstate <= se_while2;
			DATA_RDWR <= '1';
            DATA_EN <= '1';
            sel1 <= '0';

        when se_while2 =>
			nstate <= sfetch0;
			if (DATA_RDATA = "00000000") then
				PC_inc <= '1';
				PC_dec <= '0';
            else
				nstate <= se_while3;
                CNT_inc <= '1';
				CNT_dec <= '0';
				PC_dec <= '1';
				PC_inc <= '0';
            end if;
            
        when se_while3 =>
            nstate <= se_while4;  
            sel1 <= '1';
            DATA_EN <= '1';
			DATA_RDWR <= '1';   

        when se_while4 =>
            if (CNT_reg = "00000000") then
               nstate <= sfetch0;
            else
               if (DATA_RDATA = X"5B") then
					CNT_dec <= '1';
					CNT_inc <= '0';
               elsif (DATA_RDATA = X"5D") then
					CNT_inc <= '1';
					CNT_dec <= '0';
               end if;
            nstate <= se_while5;
            end if;

        when se_while5 =>
            nstate <= se_while3;  
            if (CNT_reg = "00000000") then
               PC_inc <= '1';
			   PC_dec <= '0';
            else
               PC_dec <= '1';
			   PC_inc <= '0';
            end if;   
			
		when others => nstate <= sfetch0;
	end case;
end process;
end behavioral;	
 
