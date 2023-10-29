import { Component, OnInit } from '@angular/core';

import { ApiService } from '../api.service';

import { apiProgram, apiProgramMashStep, apiProgramHopStep,
	 apiStateData } from '../api.types';

@Component({
  selector: 'app-brew',
  templateUrl: './brew.component.html',
  styleUrls: ['./brew.component.css']
})
export class BrewComponent implements OnInit {

  public program: apiProgram | null = null;

  public sensors = [];

  public state: string | null = null;
  public needmalt: boolean = false;
  public targettemp: number = 0;
  public mashstep: apiProgramMashStep | null = null;
  public bktemp:number | null = null;
  public hopping: apiProgramHopStep | null = null;

  public coolingdone: boolean = false;
  public forcepump: boolean = false;
  public blockheat: boolean = false;
  public bkpump: boolean = false;

  // cooling temperature
  public cooltemp: number | null = null;

  // volume
  public volume: number = 42;

  constructor(private api: ApiService) {
  }

  ngOnInit() {
    this.api.getState()
      .subscribe(
	(data:apiStatedata) => {
	  this.updateState(data);
	});
    this.api.temphistory$
      .subscribe(
	(data: apiBrewTempHistoryData|null) => {
	  if ( data == null ) this.temphistory = null;
	  else this.updateTempHistory(data);
	});
    this.onVolumeReset();
  }

  updateTempHistory(data: apiBrewTempHistoryData) {
  }

}
