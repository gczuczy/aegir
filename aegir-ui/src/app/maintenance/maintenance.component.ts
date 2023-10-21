import { Component } from '@angular/core';

import { ApiService } from '../api.service';

import { apiStateData } from '../api.types';

interface sensorData {
  sensor: string,
  temp: string
};

@Component({
  selector: 'app-maintenance',
  templateUrl: './maintenance.component.html',
  styleUrls: ['./maintenance.component.css']
})
export class MaintenanceComponent {

  public sensors: sensorData[] = [];

  canmaint:boolean = false;
  inmaint:boolean = false;

  pumpon:boolean = false;
  bkpumpon:boolean = false;
  heaton:boolean = false;
  heattemp:number = 37.0;

  constructor(private api: ApiService) {
  }

  ngOnInit() {
    this.api.getState().subscribe(
      (data:apiStateData) => {
	this.updateState(data);
      }
    );
  }

  updateState(data: apiStateData) {
    let state = data.state;

    this.canmaint = (state == "Empty" || state == "Finished" || state == "Maintenance");
    this.inmaint = (state == "Maintenance");

    let newsensors : sensorData[] = []

    for (const [key, value] of Object.entries(data.currtemp)) {
      let temp = parseFloat(value);
      if ( temp > 1000 ) temp = 0.0;
      newsensors.push({'sensor': key,
		       'temp': temp.toFixed(2)});
    }
    this.sensors = newsensors;
  }

  startMaintenance() {
    this.api.startMaintenance().subscribe();
  }

  stopMaintenance() {
    this.api.stopMaintenance().subscribe();
  }

  onPumpChange(event:any) {
    this.pumpon = event['checked'];

    if ( this.heaton ) this.pumpon = true;

    this.api.setMaintenance(this.pumpon, this.heaton, this.bkpumpon, this.heattemp).subscribe();
  }

  onBKPumpChange(event:any) {
    this.bkpumpon = event['checked'];
    this.api.setMaintenance(this.pumpon, this.heaton, this.bkpumpon, this.heattemp).subscribe();
  }

  onHeatChange(event:any) {
    this.heaton = event['checked'];
    if ( this.heaton ) this.pumpon = true;
    this.api.setMaintenance(this.pumpon, this.heaton, this.bkpumpon, this.heattemp).subscribe();
  }

  onTempSet() {
    this.api.setMaintenance(this.pumpon, this.heaton, this.bkpumpon, this.heattemp).subscribe();
  }
}
