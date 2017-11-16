import { Component, OnInit } from '@angular/core';
import 'rxjs/add/operator/switchMap';

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
	//console.log('state', data);
	this.sensors = [];
	this.state = data['state'];
	this.targettemp = data['targettemp'];
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
	for (let key in data['currtemp']) {
	    let temp = data['currtemp'][key];
	    if ( temp > 1000 ) temp = 0.0;
	    this.sensors.push({'sensor': key, 'temp': parseFloat(temp).toFixed(2)});
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
    }

    swissCheese(a) {
	let size:number = a.length;
	let skiplog:Array<number> = [];
	for ( let i in a ) {
	    let ni:number = +i;
	    let age:number = size-ni;
	    if ( ni == 0 ) continue;
	    if ( age > 3600 ) {
		if ( ni%180 != 0 ) {
		    skiplog.push(a[i]);
		    a[i] = null;
		} else if ( skiplog.length > 0 ) {
		    let avg = skiplog.reduce((a,b)=> a+b,0) / skiplog.length;
		    //console.log(skiplog, avg);
		    skiplog.length = 0;
		    a[i] = avg;
		}
	    } else if ( age > 600 ) {
		if ( ni%60 != 0 ) {
		    skiplog.push(a[i]);
		    a[i] = null;
		} else if ( skiplog.length > 0 ) {
		    let avg = skiplog.reduce((a,b)=> a+b,0) / skiplog.length;
		    skiplog.length = 0;
		    a[i] = avg;
		}
	    } else if ( age > 180 ) {
		if ( ni%10 != 0 ) {
		    skiplog.push(a[i]);
		    a[i] = null;
		} else if ( skiplog.length > 0 ) {
		    let avg = skiplog.reduce((a,b)=> a+b,0) / skiplog.length;
		    skiplog.length = 0;
		    a[i] = avg;
		}
	    }
	}
	return a;
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

    onAbortBrew() {
	if ( confirm("This will cancel the brew process, are you sure?") ) {
	    this.api.abortBrew().subscribe();
	}
    }

    onVolumeReset() {
	this.api.getVolume().subscribe(data => {
	    this.volume = data['volume'];
	});
    }

    onVolumeSet() {
	this.api.setVolume(this.volume)
	    .subscribe(data => console.log(data),
		       err => console.log(err));

	// and finally display the newly active volume
	this.onVolumeReset();
    }
}
