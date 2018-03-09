import { Component, OnInit } from '@angular/core';

import { ApiService } from './api.service';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit {
    title = 'Aegir';

    canmaint:boolean = true;

    constructor(private api: ApiService) {
    }

    ngOnInit() {
	this.api.getState().subscribe(data => {
	    this.updateState(data);
	});
    }

    updateState(data) {
	let state = data['state'];

	this.canmaint = (state == "Empty" || state == "Finished" || state == "Maintenance");
	//console.log(data, this.canmaint);
    }
}
