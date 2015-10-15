# THIS FILE IS AUTOMATICALLY GENERATED
# Project: D:\Users\gizmo\Documents\PSoC\PSoC_Fixedpoint_DDS_RhythmMachine\PSoC_Fixedpoint_DDS_RhythmMachine_Test01.cydsn\PSoC_Fixedpoint_DDS_RhythmMachine_Test01.cyprj
# Date: Thu, 15 Oct 2015 22:01:45 GMT
#set_units -time ns
create_clock -name {Clock_1(FFB)} -period 100000 -waveform {0 50000} [list [get_pins {ClockBlock/ff_div_8}]]
create_clock -name {UART_SCBCLK(FFB)} -period 729.16666666666663 -waveform {0 364.583333333333} [list [get_pins {ClockBlock/ff_div_3}]]
create_clock -name {CyRouted1} -period 20.833333333333332 -waveform {0 10.4166666666667} [list [get_pins {ClockBlock/dsi_in_0}]]
create_clock -name {CyILO} -period 31250 -waveform {0 15625} [list [get_pins {ClockBlock/ilo}]]
create_clock -name {CyLFCLK} -period 31250 -waveform {0 15625} [list [get_pins {ClockBlock/lfclk}]]
create_clock -name {CyIMO} -period 20.833333333333332 -waveform {0 10.4166666666667} [list [get_pins {ClockBlock/imo}]]
create_clock -name {CyHFCLK} -period 20.833333333333332 -waveform {0 10.4166666666667} [list [get_pins {ClockBlock/hfclk}]]
create_clock -name {CySYSCLK} -period 20.833333333333332 -waveform {0 10.4166666666667} [list [get_pins {ClockBlock/sysclk}]]
create_generated_clock -name {Clock_1} -source [get_pins {ClockBlock/hfclk}] -edges {1 4801 9601} [list]
create_generated_clock -name {UART_SCBCLK} -source [get_pins {ClockBlock/hfclk}] -edges {1 35 71} [list]


# Component constraints for D:\Users\gizmo\Documents\PSoC\PSoC_Fixedpoint_DDS_RhythmMachine\PSoC_Fixedpoint_DDS_RhythmMachine_Test01.cydsn\TopDesign\TopDesign.cysch
# Project: D:\Users\gizmo\Documents\PSoC\PSoC_Fixedpoint_DDS_RhythmMachine\PSoC_Fixedpoint_DDS_RhythmMachine_Test01.cydsn\PSoC_Fixedpoint_DDS_RhythmMachine_Test01.cyprj
# Date: Thu, 15 Oct 2015 22:01:43 GMT
