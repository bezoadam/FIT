--Vypracoval: Adam Bezak xbezak01
--datum: 12.11.2015

--knihovny
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_arith.all;
use IEEE.std_logic_unsigned.all;

--popis rozhrani
entity ledc8x8 is
	port(
		SMCLK: in std_logic;
		ROW: out std_logic_vector (0 to 7);
		LED: out std_logic_vector (0 to 7);
		RESET: in std_logic
	);
end entity ledc8x8;

--realizacia rozhrani
architecture behavior of ledc8x8 is
	signal row_cnt: STD_LOGIC_VECTOR (0 to 7) := "10000000";
	signal sw : STD_LOGIC;
	signal rowLEDs : STD_LOGIC_VECTOR (0 to 7);
	signal clock : STD_LOGIC;
	signal clock_cnt : STD_LOGIC_VECTOR (0 to 7);
	signal i_count : STD_LOGIC_VECTOR (21 downto 0);
begin
	
signal_div: process (RESET, SMCLK)
	begin

		if RESET = '1' then
			clock_cnt <= "00000000";
			sw <= '0';
		elsif SMCLK'event and SMCLK = '1' then
			if sw = '0' then
        i_count <= i_count + 1;
			 if i_count = "1111111111111111111111" then
				  sw <= '1';
				  i_count <= (others => '0');
			 end if;
      	end if;
			clock_cnt <= clock_cnt + 1;
			if clock_cnt = "11111111" then
				clock <= '1';
				clock_cnt <= (others => '0');
			else 
				clock <= '0';
			end if;
		end if;

end process;

	lines: process (RESET, SMCLK, clock)
	begin
		if RESET = '1' then
			row_cnt <= "10000000";
		elsif SMCLK'event and SMCLK = '1' then
			if clock = '1' then
			    case row_cnt is         
						when "00000001" => row_cnt <= "10000000";
						when "00000010" => row_cnt <= "00000001";
						when "00000100" => row_cnt <= "00000010";
						when "00001000" => row_cnt <= "00000100";
						when "00010000" => row_cnt <= "00001000";
						when "00100000" => row_cnt <= "00010000";
						when "01000000" => row_cnt <= "00100000";
						when "10000000" => row_cnt <= "01000000";	
						when others => null;
				end case;
			end if;
		end if;
	end process;

	on_lines_1: process (row_cnt, sw, RESET)
	begin
		if RESET = '1' then
			rowLEDs <= "11111111";
		elsif sw = '0' then
			case row_cnt is
				when "10000000" => rowLEDs <= "10011111";
				when "01000000" => rowLEDs <= "01101111";
				when "00100000" => rowLEDs <= "00001111";
				when "00010000" => rowLEDs <= "01101111";
				when "00001000" => rowLEDs <= "01101111";
				when "00000100" => rowLEDs <= "11111111";
				when "00000010" => rowLEDs <= "11111111";
				when "00000001" => rowLEDs <= "11111111";
				when others => null;
			end case;
		else
			case row_cnt is
				when "10000000" => rowLEDs <= "10011111";
				when "01000000" => rowLEDs <= "01101111";
				when "00100000" => rowLEDs <= "00001111";
				when "00010000" => rowLEDs <= "01100001";
				when "00001000" => rowLEDs <= "01100110";
				when "00000100" => rowLEDs <= "11110001";
				when "00000010" => rowLEDs <= "11110110";
				when "00000001" => rowLEDs <= "11110001";
				when others => null;
			end case;
		end if;
	end process;

ROW <= row_cnt;
LED <= rowLEDs;

end architecture behavior;