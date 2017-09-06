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

    constructor(private api: ApiService) {
    }

    ngOnInit() {
	this.api.getState().subscribe(data => {
	    this.updateState(data);
	});
    }

    updateState(data) {
	console.log('BrewComponent updating data: ', data);
	this.sensors = [];
	for (let key in data['currtemp']) {
	    this.sensors.push({'sensor': key, 'temp': data['currtemp'][key]});
	}
	console.log(this.sensors);
    }

}
