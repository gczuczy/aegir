import { Component, OnInit } from '@angular/core';

import { ApiService } from '../api.service';

@Component({
  selector: 'app-maintenance',
  templateUrl: './maintenance.component.html',
  styleUrls: ['./maintenance.component.css']
})
export class MaintenanceComponent implements OnInit {

    canmaint:boolean = false;
    inmaint:boolean = false;

    pumpon:boolean = false;
    heaton:boolean = false;
    heattemp:number = 37.0;

    public sensors = [];

    constructor(private api: ApiService) {
    }

    ngOnInit() {
	this.api.getState().subscribe(data => {
	    this.updateState(data);
	});
    }

    updateState(data) {
	let state = data['state'];
	this.sensors = [];

	this.canmaint = (state == "Empty" || state == "Finished" || state == "Maintenance");
	this.inmaint = (state == "Maintenance");
	for (let key in data['currtemp']) {
	    let temp = data['currtemp'][key];
	    if ( temp > 1000 ) temp = 0.0;
	    this.sensors.push({'sensor': key, 'temp': parseFloat(temp).toFixed(2)});
	}
	//console.log(data, this.canmaint);
    }

    startMaintenance() {
	this.api.startMaintenance().subscribe();
    }

    stopMaintenance() {
	this.api.stopMaintenance().subscribe();
    }

    onPumpChange(event) {
	// event is the new value
	//console.log('onPumpChange', event);
	this.pumpon = event;
	if ( !event ) {
	    this.heaton = false;
	}
	this.api.setMaintenance(this.pumpon, this.heaton, this.heattemp).subscribe();
    }

    onHeatChange(event) {
	// event is the new value
	//console.log('onHeatChange', event);
	this.heaton = event;
	if ( event ) {
	    this.pumpon = true;
	}
	this.api.setMaintenance(this.pumpon, this.heaton, this.heattemp).subscribe();
    }

    onTempSet() {
	this.api.setMaintenance(this.pumpon, this.heaton, this.heattemp).subscribe();
    }
}
