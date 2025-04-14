#!/bin/sh

	echo "=========== updating smp affinity ============"
	smp_affinity=4
	irq_affinity_num=`grep -E -m3 'wlan' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo $smp_affinity > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m2 'wlan' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo $smp_affinity > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'wlan' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo $smp_affinity > /proc/irq/$irq_affinity_num/smp_affinity

	irq_affinity_num=`grep -E -m1 'reo2host-destination-ring2' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 2 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'reo2host-destination-ring1' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 1 > /proc/irq/$irq_affinity_num/smp_affinity

	#Keep smp affinity for other Lithium chips across all 4 cores.
	irq_affinity_num=`grep -E -m1 'reo2host-destination-ring4' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 8 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'reo2host-destination-ring3' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 4 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'wbm2host-tx-completions-ring4' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 8 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'wbm2host-tx-completions-ring3' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 4 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'wbm2host-tx-completions-ring2' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 2 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'wbm2host-tx-completions-ring1' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 1 > /proc/irq/$irq_affinity_num/smp_affinity

	irq_affinity_num=`grep -E -m1 'ppdu-end-interrupts-mac1' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 8 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'rxdma2host-monitor-status-ring-mac1' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 8 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'rxdma2host-monitor-destination-mac1' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 8 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'host2rxdma-monitor-ring1' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 8 > /proc/irq/$irq_affinity_num/smp_affinity

	irq_affinity_num=`grep -E -m1 'ppdu-end-interrupts-mac2' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 4 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'rxdma2host-monitor-status-ring-mac2' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 4 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'rxdma2host-monitor-destination-mac2' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 4 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'host2rxdma-monitor-ring2' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 4 > /proc/irq/$irq_affinity_num/smp_affinity

	irq_affinity_num=`grep -E -m1 'ppdu-end-interrupts-mac3' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 2 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'rxdma2host-monitor-status-ring-mac3' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 2 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'rxdma2host-monitor-destination-mac3' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 2 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'host2rxdma-monitor-ring3' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 2 > /proc/irq/$irq_affinity_num/smp_affinity

	# Enable smp affinity for PCIE attach
	#pci 1
	irq_affinity_num=`grep -E -m1 'pci1_wlan_grp_dp_0' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 1 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'pci1_wlan_grp_dp_1' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 2 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'pci1_wlan_grp_dp_2' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 4 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'pci1_wlan_grp_dp_3' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 8 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'pci1_wlan_grp_dp_4' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 1 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'pci1_wlan_grp_dp_5' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 2 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'pci1_wlan_grp_dp_6' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 4 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'pci1_wlan_grp_dp_7' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 8 > /proc/irq/$irq_affinity_num/smp_affinity


	irq_affinity_num=`grep -E -m1 'pci1_wlan_grp_dp_4' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 1 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'pci1_wlan_grp_dp_5' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 2 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'pci1_wlan_grp_dp_6' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 4 > /proc/irq/$irq_affinity_num/smp_affinity
	irq_affinity_num=`grep -E -m1 'pci1_wlan_grp_dp_7' /proc/interrupts | cut -d ':' -f 1 | tail -n1 | tr -d ' '`
	[ -n "$irq_affinity_num" ] && echo 8 > /proc/irq/$irq_affinity_num/smp_affinity

