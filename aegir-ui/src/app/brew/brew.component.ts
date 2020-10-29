import { Component, OnInit } from '@angular/core';
import { HttpErrorResponse } from '@angular/common/http';

import { Program } from '../programs/program';
import { ApiService } from '../api.service';

@Component({
  selector: 'app-brew',
  templateUrl: './brew.component.html',
  styleUrls: ['./brew.component.css']
})
export class BrewComponent implements OnInit {
    public program: Program = null;

    public sensors = [];

    public state: string = null;
    public needmalt = false;
    public targettemp = 0;
    public mashstep = null;
    public bktemp:number = null;
    public hopping = null;

    public coolingdone = false;
    public forcepump = false;
    public blockheat = false;
    public bkpump:boolean = false;

    // cooling temperature
    public cooltemp:number = null;

    // volume
    public volume: number = 42;

    // chart
    public brewChartData: Array<any> = [
	{data: [], label: 'Mash Tun'},
	{data: [], label: 'RIMS Tube'}];
    public brewChartLabels: Array<any> = [];
    public brewChartOptions: any = {
	responsive: true,
	spanGaps: true,
	title: {
	    display: true,
	    text: 'RIMS and MashTun temperature'
	},
	scales: {
	    xAxes: [{
		ticks: {
		    callback: function (value, index, values) {
			let second = value % 60;
			let minute = (value-second) / 60;
			return minute + 'm' + second + 's';
		    }
		}
	    }],
	    yAxes: [{
		ticks: {
		    beginAtZero: true
		}
	    }]
	}
    };
    public brewChartLegend:boolean = true;
    public brewChartType:string = 'line';
    public brewChartColors: Array<any> = [
	'blue', 'red'
    ];

    constructor(private api: ApiService) {
    }

    ngOnInit() {
	this.api.getState().subscribe(data => {
	    this.updateState(data);
	});
	this.api.getTempHistory().subscribe(
	    data => {
		this.updateTempHistory(data);
	    });
	this.onVolumeReset();
    }

    updateState(data) {
	this.sensors = [];
	this.state = data['state'];
	this.targettemp = data['targettemp'];
	//console.log('brew::updateState', data);

	// mashing data
	if ( 'mashstep' in data ) {
	    this.mashstep = data['mashstep'];
	    let mstime = this.mashstep.time;
	    let secs = mstime % 60;
	    mstime -= secs;
	    let minutes = mstime / 60;
	    this.mashstep['textual'] = minutes + 'm ' + secs + 's';
	} else {
	    this.mashstep = null;
	}

	if ( 'cooling' in data ) {
	    this.coolingdone = data['cooling']['ready'];
	    if ( this.cooltemp == null ) {
		this.cooltemp = data['cooling']['cooltemp'];
	    }
	} else {
	    this.coolingdone = false;
	}

	for (let key in data['currtemp']) {
	    let temp = data['currtemp'][key];
	    if ( temp > 1000 ) temp = 0.0;
	    this.sensors.push({'sensor': key, 'temp': parseFloat(temp).toFixed(2)});
	    if ( key == "BK" ) {
		this.bktemp = parseFloat(temp);
	    }
	}

	// if we have the programid, then load the program as well
	if ( 'programid' in data && this.program == null) {
	    this.api.getProgram(data['programid']).subscribe(
		progdata => {
		    this.program = progdata['data'];
		    //console.log('program', this.program);
		}
	    );
	}
	if ( !('programid' in data) ) {
	    this.program = null;
	}

	// hopping data
	if ( 'hopping' in data && this.program != null) {
	    let hoptime = data['hopping']['hoptime']
	    this.hopping = {'schedule': [],
			    'hoptime': hoptime};
	    for (let hop of this.program.hops ) {
		let tth = hoptime - (hop['attime']*60);
		let hopdone = false;
		if ( tth < 0 ) hopdone = true;
		//console.log('hop', hop, tth, hopdone);
		let second = tth % 60;
		let minute = (tth-second) / 60;
		let tthstr =  minute + 'm' + second + 's';
		this.hopping['schedule'].push({'id': hop['id'],
					       'name': hop['hopname'],
					       'qty': hop['hopqty'],
					       'done': hopdone,
					       'tth': tthstr})
	    }
	    //console.log(this.hopping, this.program);
	} else {
	    //console.log('No hopping', data, this.program);
	    this.hopping = null;
	}
    }

    swissCheese(a) {
	//console.log('swissCheese input', a);
	let ret:Array<number> = [];
	let size:number = a.length;
	let skiplog:Array<number> = [];
	let rate:number = Math.sqrt(Math.log10(a.length));
	let maxstep:number = Math.ceil(Math.log(a.length));
	let next:number;
	const add = (x,y) => x+y;
	//let avg:number = skiplog.reduce(add);
	//console.log('rate', rate, maxstep);
	for ( let i in a ) {
	    let idx:number = a.length - Number(i) -1;
	    if ( Number(i) == 0 ) {
		ret.push(a[i]);
		next = Math.trunc(idx - Math.log(idx));
		//console.log('init next', next, idx, rate, (idx/rate));
		continue;
	    }
	    if ( idx == next || idx == 0) {
		skiplog.push(a[i]);
		let avg:number = skiplog.reduce(add);
		avg /= skiplog.length;
		skiplog.length = 0;
		ret.push(avg);
		next = Math.max(Math.ceil(idx - Math.pow(Math.log(idx), rate)), maxstep);
		//console.log('next next', next, idx, rate, (idx/rate));
	    } else {
		skiplog.push(a[i]);
		ret.push(null);
	    }
	}
	//console.log('swissCheese out', ret);
	return ret;
    }

    updateTempHistory(data) {
	//console.log('Updating temphistory with ', data);

	// first clear the data

	let mt:Array<number> = [];
	let rims:Array<number> = [];

	for (let key in data['readings']) {
	    if ( key == 'MashTun' ) {
		mt = data['readings'][key];
	    } else if ( key == 'RIMS' ) {
		rims = data['readings'][key];
	    }
	}
	this.brewChartData = [
	    {data: this.swissCheese(mt), label: 'Mash Tun'},
	    {data: this.swissCheese(rims), label: 'RIMS Tube'}
	];
	this.brewChartLabels.length = 0;
	for ( let i of data['timestamps'] ) {
	    this.brewChartLabels.push(i);
	}
	//console.log('chartdata', this.brewChartData);
	//console.log('chart labels', this.brewChartLabels);
    }

    onHasMalts() {
	this.api.hasMalt().subscribe();
    }

    onSpargeDone() {
	this.api.spargeDone().subscribe();
    }

    onCoolingDone() {
	this.api.coolingDone().subscribe();
    }

    onTransferDone() {
	this.api.transferDone().subscribe();
    }

    onAbortBrew() {
	if ( confirm("This will cancel the brew process, are you sure?") ) {
	    this.api.abortBrew().subscribe();
	}
    }

    onVolumeReset() {
	this.api.getVolume().subscribe(data => {
	    this.volume = data['volume'];
	},
				       error => {
					   console.log('volume err', error);
				       });
    }

    onVolumeSet() {
	this.api.setVolume(this.volume)
	    .subscribe(data => console.log(data),
		       err => console.log(err));

	// and finally display the newly active volume
	this.onVolumeReset();
    }

    onStartBoil() {
	this.api.startBoil()
	    .subscribe(data => console.log(data));
    }

    onBlockHeat(event) {
	// event is the new value
	this.blockheat = event['checked'];
	this.api.override(this.blockheat, this.forcepump, this.bkpump).subscribe();
    }

    onForcePump(event) {
	// event is the new value
	this.forcepump = event['checked'];
	this.api.override(this.blockheat, this.forcepump, this.bkpump).subscribe();
    }

    onBKPump(event) {
	// event is the new value
	this.bkpump = event['checked'];
	this.api.override(this.blockheat, this.forcepump, this.bkpump).subscribe();
    }

    onCoolTempSet() {
	this.api.setCoolTemp(this.cooltemp)
	    .subscribe(data => console.log(data),
		       err => console.log(err));

	// and finally display the newly active volume
	this.onVolumeReset();
    }
}
