import { Component, OnInit, ViewChild } from '@angular/core';
import { BaseChartDirective } from 'ng2-charts';
import { ChartConfiguration, ChartOptions, ChartType } from 'chart.js';
import { FontAwesomeModule } from '@fortawesome/angular-fontawesome';
import { faRefresh, faCheck, faHourglass, faEllipsisH } from '@fortawesome/free-solid-svg-icons';

import { ApiService } from '../api.service';

import { apiProgram,
	 apiStateData, apiBrewTempHistory,
	 apiBrewStateVolumeData,
	 apiStateMashStep, apiStateHopping
       } from '../api.types';

import { Enzyme, activeEnzymes } from '../enzymes';

interface sensorValue {
  sensor: string,
  temp: string
};

interface ChartData {
  data: Array<number|null>,
  label: string,
};

@Component({
  selector: 'app-brew',
  templateUrl: './brew.component.html',
  styleUrls: ['./brew.component.css']
})
export class BrewComponent implements OnInit {
  // fontawesome
  faRefresh = faRefresh;
  faCheck = faCheck;
  faHourglass = faHourglass;
  faEllipsisH = faEllipsisH;


  @ViewChild(BaseChartDirective)
  public chart?: BaseChartDirective;

  public program: apiProgram | null = null;

  public sensors: sensorValue[] = [];

  public state: string | null = null;
  public needmalt: boolean = false;
  public targettemp: number = 0;
  public brewtime?: string;
  public mashstep: apiStateMashStep | null = null;
  public bktemp:number | null = null;
  public hopping: apiStateHopping | null = null;
  public rest: string | null = null;

  public coolingdone: boolean = false;
  public forcepump: boolean = false;
  public blockheat: boolean = false;
  public bkpump: boolean = false;

  // cooling temperature
  public cooltemp: number | null = null;

  // volume
  public volume: number = 42;

  // Chart
  public brewChartLegend:boolean = true;
  public brewChartData: ChartConfiguration<'line'>['data'] = {
    labels: [],
    datasets: []
  };
  public brewChartOptions: ChartOptions<'line'> = {
    animation: false,
    animations: {
      colors: false,
    },
    transitions: {
      active: {
	animation: {
	  duration: 0,
	},
      },
    },
    responsive: true,
    spanGaps: true,
    scales: {
      yAxis: {
	beginAtZero: true,
	min: 0,
	max: 90,
	title: {
	  display: true,
	  text: '[C]',
	},
      },
      xAxis: {
	title: {
	  display: true,
	  text: 'Elapsed time',
	},
      },
    },
    plugins: {
      title: {
	display: true,
	text: 'Mash Tun and RIMS temperature',
      },
    },
  };

  constructor(private api: ApiService) {
  }

  ngOnInit() {
    this.api.getState()
      .subscribe(
	(data:apiStateData) => {
	  this.updateState(data);
	});
    this.api.temphistory$
      .subscribe(
	(data: apiBrewTempHistory|null) => {
	  if ( data ) this.updateTempHistory(data);
	});
    this.onVolumeReset();
    //console.log('brewChartData', this.brewChartData);
  }

  updateState(data:apiStateData) {
    this.sensors = [];
    this.state = data['state'];
    this.targettemp = data['targettemp'];
    //console.log('brew::updateState', data);

    // mashing data
    if ( 'mashstep' in data ) {
      this.mashstep = data.mashstep as apiStateMashStep;
      let mstime = this.mashstep!.time;
      let secs = mstime % 60;
      mstime -= secs;
      let minutes = mstime / 60;
      this.mashstep!['textual'] = minutes + 'm ' + secs + 's';
    } else {
      this.mashstep = null;
    }

    if ( 'cooling' in data ) {
      this.coolingdone = data.cooling!.ready;
      if ( this.cooltemp == null ) {
	this.cooltemp = data.cooling!.cooltemp;
      }
    } else {
      this.coolingdone = false;
    }

    Object.entries(data.currtemp)
      .forEach(
	([key, value]) => {
	  let temp:number = value;
	  if ( temp > 1000 ) temp = 0.0;
	  this.sensors.push({'sensor': key, 'temp': temp.toFixed(2)});
	  if ( key == "BK" ) {
	    this.bktemp = temp;
	  }
	}
      );

    // active enzyme
    if ( this.bktemp != null ) {
      let ae = activeEnzymes(this.bktemp);
      if ( ae.length>0 ) {
	this.rest = ae.map(
	  (data:Enzyme) => data.name
	).join(', ');
      } else {
	this.rest = null;
      }
    }

    // if we have the programid, then load the program as well
    if ( 'programid' in data && this.program == null) {
      this.api.getProgram(data.programid as number).subscribe(
	(progdata:apiProgram) => {
	  this.program = progdata;
	  //console.log('program', this.program);
	}
      );
    }
    if ( !('programid' in data) ) {
      this.program = null;
    }

    // hopping data
    if ( 'hopping' in data && this.program != null) {
      let hoptime = data.hopping!.hoptime;
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
	this.hopping!.schedule!.push({'id': hop.id as number,
				      'name': hop.hopname,
				      'qty': hop.hopqty,
				      'done': hopdone,
				      'tth': tthstr})
      }
      //console.log('hopping', this.hopping, this.program);
    } else {
      //console.log('No hopping', data, this.program);
      this.hopping = null;
    }
  }

  swissCheese(a: number[]): Array<number|null> {
    //console.log('swissCheese input', a);
    let ret:Array<number|null> = [];
    let size:number = a.length;
    let skiplog:Array<number> = [];
    let rate:number = Math.sqrt(Math.log10(a.length));
    let maxstep:number = Math.ceil(Math.log(a.length));
    let next:number = -1;
    const add = (x:number,y:number) => x+y;
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

  updateTempHistory(data: apiBrewTempHistory) {
    //console.log('Updating temphistory with ', data);

    // get the merged data from the apiService
    let th = this.api.getAllTempHistory();
    //console.log('brew::updateTempHistroy alldata', th);

    let mt = this.swissCheese(th.mt);
    let rims = this.swissCheese(th.rims);

    let mtdata: number[] = [];
    let rimsdata: number[] = [];
    let dtdata: string[] = [];
    for ( let i in th.dt ) {
      if ( mt[i] != null && rims[i] != null ) {
	mtdata.push(mt[i] as number);
	rimsdata.push(rims[i] as number);
	let second = th.dt[i] % 60;
	let minute = (th.dt[i]-second) / 60;
	dtdata.push(minute + 'm' + second + 's');
      }
    }

    this.brewtime = dtdata[dtdata.length-1];

    this.brewChartData.datasets = [
      {data: mtdata,
       tension: 0.4,
       fill: false,
       label: 'Mash Tun'},
      {data: rimsdata,
       tension: 0.4,
       fill: false,
       label: 'RIMS Tube'}
    ];
    this.brewChartData.labels = dtdata;
    this.chart!.chart!.update('resize')
    //console.log('chartdata', this.brewChartData);
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
    this.api.getVolume().subscribe(
      (data:apiBrewStateVolumeData) => {
	this.volume = data['volume'];
      },
      (error:any) => {
	console.log('volume err', error);
      });
  }

  onVolumeSet() {
    this.api.setVolume(this.volume)
      .subscribe((data:any) => console.log(data),
		 (err:any) => console.log(err));

    // and finally display the newly active volume
    this.onVolumeReset();
  }

  onStartBoil() {
    this.api.startBoil()
      .subscribe((data:any) => console.log(data));
  }

  onBlockHeat(event:any) {
    // event is the new value
    this.blockheat = event['checked'];
    this.api.override(this.blockheat, this.forcepump, this.bkpump).subscribe();
  }

  onForcePump(event:any) {
    // event is the new value
    this.forcepump = event['checked'];
    this.api.override(this.blockheat, this.forcepump, this.bkpump).subscribe();
  }

  onBKPump(event:any) {
    // event is the new value
    this.bkpump = event['checked'];
    this.api.override(this.blockheat, this.forcepump, this.bkpump).subscribe();
  }

  onCoolTempSet() {
    this.api.setCoolTemp(this.cooltemp as number)
      .subscribe((data:any) => console.log(data),
		 (err:any) => console.log(err));

    // and finally display the newly active volume
    this.onVolumeReset();
  }

}
