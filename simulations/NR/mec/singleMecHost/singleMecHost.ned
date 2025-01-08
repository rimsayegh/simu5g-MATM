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

package simu5g.simulations.NR.mec.singleMecHost;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import simu5g.world.radio.LteChannelControl;
import inet.node.inet.StandardHost;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.mec.MECHost;
import simu5g.nodes.mec.MECOrchestrator.MecOrchestrator;
import simu5g.nodes.mec.UALCMP.UALCMP;
import simu5g.nodes.backgroundCell.BackgroundCell;
import simu5g.world.radio.LteChannelControl;

network singleMecHost
{
    parameters:
        int numUe = default(1);
        int numBgCells = default(0);
        @display("i=block/network2;bgb=2500.0225,2500.6475;bgi=background/pisa");


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
            @display("p=1300.59,131.04");
        }
        iUpf: Upf {
            @display("p=1500,247.06499");
        }
        gnb: gNodeB {
            @display("p=249.795,395.85;is=vl");
        }
        bgCell[numBgCells]: BackgroundCell {
            @display("p=249.795,577.39496;is=vl");
        }
        ue[numUe]: NRUe {
            @display("p=1500,481.845");
        }

        //# MEC modules
        mecHost: MECHost {
            @display("p=1350,255.408");
        }

        mecOrchestrator: MecOrchestrator {
            @display("p=900,73.71");
        }
        ualcmp: UALCMP {
            @display("p=1200,73.71");
        }

    connections:

        upf.pppg++ <--> Eth10G <--> iUpf.pppg++;
        iUpf.pppg++ <--> Eth10G <--> gnb.ppp;
        ualcmp.ppp++ <--> Eth10G <--> upf.filterGate;

        ualcmp.toMecOrchestrator --> mecOrchestrator.fromUALCMP;
        ualcmp.fromMecOrchestrator <-- mecOrchestrator.toUALCMP;

        mecHost.ppp++ <--> Eth10G <--> iUpf.pppg++;
}
