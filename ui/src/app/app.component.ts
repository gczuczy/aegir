import { Component } from '@angular/core';
import { FontAwesomeModule } from '@fortawesome/angular-fontawesome';
import { faGear } from '@fortawesome/free-solid-svg-icons';

import { ApiService } from './api.service';

import { apiStateData, apiFermdData } from './api.types';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent {
  title = 'Aegir';
  faGear = faGear;

  canmaint:boolean = true;

  public fermds: apiFermdData[]|null = null;

  constructor(private api: ApiService) {
  }

  ngOnInit() {
    this.canmaint = false;
    this.api.getState().subscribe(data => {
      this.updateState(data);
    });

    this.api.fermds$.subscribe(
      (data:apiFermdData[]|null) => {
	this.fermds = data;
      }
    );
  }

  updateState(data: apiStateData) {
    let state = data.state;
    this.canmaint = (state == "Empty" || state == "Finished" || state == "Maintenance");
  }
}
