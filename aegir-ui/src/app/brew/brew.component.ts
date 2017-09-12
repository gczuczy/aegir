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
    public program: Program;

    public sensors = [];

    public state: string = null;
    public needmalt = false;

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
    }

    updateState(data) {
	this.sensors = [];
	this.state = data['state'];
	for (let key in data['currtemp']) {
	    let temp = data['currtemp'][key];
	    if ( temp > 1000 ) temp = 0.0;
	    this.sensors.push({'sensor': key, 'temp': temp});
	}
    }

    updateTempHistory(data) {
	console.log('Updating temphistory with ', data);
    }

    onHasMalts() {
	this.api.hasMalt().subscribe();
	console.log("HasMalts pushed");
    }

}
