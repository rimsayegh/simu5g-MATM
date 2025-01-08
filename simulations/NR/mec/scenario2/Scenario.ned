// 
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
// 
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself, 
// and cannot be removed from it.
// 
package simu5g.simulations.NR.mec.scenario2;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.nodes.mec.MECHost;
import simu5g.nodes.mec.MECOrchestrator.MecOrchestrator;
import simu5g.nodes.mec.UALCMP.UALCMP;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.backgroundCell.BackgroundCell;
import simu5g.world.radio.LteChannelControl;

// MultiMecHost
//
// This network includes two gNBs and two MEC hosts
//
network Scenario
{
    parameters:
        int numUe = default(1);
        int numBgCells = default(0);
        @display("i=block/network2;bgb=7201.504,3530.6401;bgi=background/pisa");
    submodules:
        channelControl: LteChannelControl {
            @display("p=50,25;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=50,75;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=50,125");
        }
        binder: Binder {
            @display("p=50,175;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=50.993748,258.7;is=s");
        }
        upf: Upf {
            @display("p=1407.248,255.408");
        }
        iUpf1: Upf {
            @display("p=275.44,776.24");
        }
        iUpf2: Upf {
            @display("p=520.83203,776.24");
        }
        iUpf3: Upf {
            @display("p=821.312,776.24");
        }
        iUpf4: Upf {
            @display("p=1116.784,776.24");
        }
        iUpf5: Upf {
            @display("p=1377.2001,776.24");
        }
        iUpf6: Upf {
            @display("p=1602.56,776.24");
        }
        iUpf7: Upf {
            @display("p=1913.056,776.24");
        }
        iUpf8: Upf {
            @display("p=2183.488,776.24");
        }
        iUpf9: Upf {
            @display("p=2453.9202,776.24");
        }
        iUpf10: Upf {
            @display("p=2694.304,776.24");
        }

        gnb1: gNodeB {
            @display("p=310.496,2093.344;is=vl");
        }
        gnb2: gNodeB {
            @display("p=1046.672,2093.344;is=vl");
        }
        gnb3: gNodeB {
            @display("p=1712.7361,2093.344;is=vl");
        }
        gnb4: gNodeB {
            @display("p=2453.9202,2093.344;is=vl");
        }
        gnb5: gNodeB {
            @display("p=3104.96,2093.344;is=vl");
        }
        gnb6: gNodeB {
            @display("p=3761.008,2093.344;is=vl");
        }
        gnb7: gNodeB {
            @display("p=4482.16,2093.344;is=vl");
        }
        gnb8: gNodeB {
            @display("p=5253.392,2093.344;is=vl");
        }
        gnb9: gNodeB {
            @display("p=6064.688,2093.344;is=vl");
        }
        gnb10: gNodeB {
            @display("p=6880.992,2093.344;is=vl");
        }

        bgCell[numBgCells]: BackgroundCell {
            @display("p=3956.32,3094.944;is=vl");
        }
        ue[numUe]: NRUe {
            @display("p=6991.168,2944.704");
        }

        //# MEC modules
        mecHost1: MECHost {
            @display("p=410.656,631.008");
        }
        mecHost2: MECHost {
            @display("p=766.224,631.008");
        }
        mecHost3: MECHost {
            @display("p=991.58405,631.008");
        }
        mecHost4: MECHost {
            @display("p=1287.056,631.008");
        }
        mecHost5: MECHost {
            @display("p=1547.472,661.056");
        }
        mecHost6: MECHost {
            @display("p=1772.832,631.008");
        }
        mecHost7: MECHost {
            @display("p=2083.3281,631.008");
        }
        mecHost8: MECHost {
            @display("p=2358.768,661.056");
        }
        mecHost9: MECHost {
            @display("p=2624.1921,631.008");
        }
        mecHost10: MECHost {
            @display("p=2794.464,661.056");
        }

        mecOrchestrator: MecOrchestrator {
            @display("p=375.6,170.272");
        }
        ualcmp: UALCMP {
            @display("p=936.49603,120.192");
        }


    connections:

        //# 5G Core Network connections
        upf.pppg++ <--> Eth10G <--> iUpf1.pppg++;
        upf.pppg++ <--> Eth10G <--> iUpf2.pppg++;
        upf.pppg++ <--> Eth10G <--> iUpf3.pppg++;
        upf.pppg++ <--> Eth10G <--> iUpf4.pppg++;
        upf.pppg++ <--> Eth10G <--> iUpf5.pppg++;
        upf.pppg++ <--> Eth10G <--> iUpf6.pppg++;
        upf.pppg++ <--> Eth10G <--> iUpf7.pppg++;
        upf.pppg++ <--> Eth10G <--> iUpf8.pppg++;
        upf.pppg++ <--> Eth10G <--> iUpf9.pppg++;
        upf.pppg++ <--> Eth10G <--> iUpf10.pppg++;

        iUpf1.pppg++ <--> Eth10G <--> gnb1.ppp;
        iUpf2.pppg++ <--> Eth10G <--> gnb2.ppp;
        iUpf3.pppg++ <--> Eth10G <--> gnb3.ppp;
        iUpf4.pppg++ <--> Eth10G <--> gnb4.ppp;
        iUpf5.pppg++ <--> Eth10G <--> gnb5.ppp;
        iUpf6.pppg++ <--> Eth10G <--> gnb6.ppp;
        iUpf7.pppg++ <--> Eth10G <--> gnb7.ppp;
        iUpf8.pppg++ <--> Eth10G <--> gnb8.ppp;
        iUpf9.pppg++ <--> Eth10G <--> gnb9.ppp;
        iUpf10.pppg++ <--> Eth10G <--> gnb10.ppp;

        //# MEC-related connections
        ualcmp.ppp++ <--> Eth10G <--> upf.filterGate;
        ualcmp.toMecOrchestrator --> mecOrchestrator.fromUALCMP;
        ualcmp.fromMecOrchestrator <-- mecOrchestrator.toUALCMP;
        mecHost1.ppp++ <--> Eth10G <--> iUpf1.pppg++;
        mecHost2.ppp++ <--> Eth10G <--> iUpf2.pppg++;
        mecHost3.ppp++ <--> Eth10G <--> iUpf3.pppg++;
        mecHost4.ppp++ <--> Eth10G <--> iUpf4.pppg++;
        mecHost5.ppp++ <--> Eth10G <--> iUpf5.pppg++;
        mecHost6.ppp++ <--> Eth10G <--> iUpf6.pppg++;
        mecHost7.ppp++ <--> Eth10G <--> iUpf7.pppg++;
        mecHost8.ppp++ <--> Eth10G <--> iUpf8.pppg++;
        mecHost9.ppp++ <--> Eth10G <--> iUpf9.pppg++;
        mecHost10.ppp++ <--> Eth10G <--> iUpf10.pppg++;


        //# X2 connections
        gnb1.x2++ <--> Eth10G <--> gnb2.x2++;
        gnb2.x2++ <--> Eth10G <--> gnb3.x2++;
        gnb3.x2++ <--> Eth10G <--> gnb4.x2++;
        gnb4.x2++ <--> Eth10G <--> gnb5.x2++;
        gnb5.x2++ <--> Eth10G <--> gnb6.x2++;
        gnb6.x2++ <--> Eth10G <--> gnb7.x2++;
        gnb7.x2++ <--> Eth10G <--> gnb8.x2++;
        gnb8.x2++ <--> Eth10G <--> gnb9.x2++;
        gnb9.x2++ <--> Eth10G <--> gnb10.x2++;
}
