import { Component, OnInit } from '@angular/core';
import { Validators, FormGroup, FormControl, AbstractControl} from '@angular/forms';
import { switchMap } from 'rxjs/operators';
import { faPlus } from '@fortawesome/free-solid-svg-icons';

import { ApiService } from '../api.service';

import { apiConfig, apiFermd } from '../api.types';

@Component({
  selector: 'app-settings',
  templateUrl: './settings.component.html',
  styleUrls: ['./settings.component.css']
})
export class SettingsComponent implements OnInit {
  faPlus = faPlus;
  public showAddFermd: boolean = false;
  public settingsForm: FormGroup = new FormGroup({
    tempaccuracy: new FormControl("", [
      Validators.required,
      (control: AbstractControl) => Validators.min(0)(control),
      (control: AbstractControl) => Validators.max(10)(control)]),
    cooltemp: new FormControl("", [Validators.required,
      (control: AbstractControl) => Validators.min(17)(control),
      (control: AbstractControl) => Validators.max(30)(control)]),
    hepower: new FormControl("", [Validators.required,
      (control: AbstractControl) => Validators.min(1000)(control),
      (control: AbstractControl) => Validators.max(15000)(control)]),
    heatoverhead: new FormControl("", [Validators.required,
      (control: AbstractControl) => Validators.min(0.1)(control),
      (control: AbstractControl) => Validators.max(5)(control)]),
    hedelay: new FormControl("", [Validators.required,
      (control: AbstractControl) => Validators.min(5)(control),
      (control: AbstractControl) => Validators.max(30)(control)]),
    loglevel: new FormControl("info", [Validators.required]),
  });
  public addFermdForm: FormGroup = new FormGroup({
    name: new FormControl("", [Validators.required,
      Validators.minLength(3)]),
    address: new FormControl("", [Validators.required,
      Validators.pattern("^tcp://[a-z0-9]+:[0-9]+$")]),
  });
  public fermdForm: FormGroup = new FormGroup({
    name: new FormControl("", [Validators.required]),
    address: new FormControl("", [Validators.required]),
  });

  public errors: string[] = [];
  public fermds: apiFermd[] = [];

  constructor(private api: ApiService) {
  }

  ngOnInit() {
    this.updateConfig();
    this.api.fermds$.subscribe(
      (data:apiFermd[]|null) => {
	if ( data != null ) {
	  this.fermds = data;
	} else {
	  this.fermds = [];
	}
      }
    );
  }

  updateConfig() {
    this.api.getConfig().subscribe(
      (res:apiConfig) => {
	this.settingsForm.patchValue({
	  hepower: res.hepower,
	  tempaccuracy: res.tempaccuracy.toFixed(2),
	  cooltemp: res.cooltemp,
	  heatoverhead: res.heatoverhead.toFixed(2),
	  hedelay: res.hedelay,
	  loglevel: res.loglevel,
	});
      },
      (err:any) => {
      }
    );
  }

  save(model: FormGroup) {
    let data: apiConfig = {
      hepower: model.get('hepower')!.value,
      tempaccuracy: model.get('tempaccuracy')!.value,
      cooltemp: model.get('cooltemp')!.value,
      heatoverhead: model.get('heatoverhead')!.value,
      hedelay: model.get('hedelay')!.value,
      loglevel: model.get('loglevel')!.value,
    };

    this.api.setConfig(data).subscribe(
      (resp:any) => {
	console.log('setConfig resp', resp);
	this.updateConfig();
      },
      (err: any) => {
	console.log('setConfig error', err);
	this.updateConfig();
      }
    );
  }

  addFermd(model: FormGroup) {
    console.log('Clicked', model);
    this.errors = [];
    let fermd: apiFermd = {
      name: model.get('name')!.value,
      address: model.get('address')!.value,
    };
    this.api.addFermd(fermd).subscribe(
      (data:apiFermd) => {
	this.api.updateFermds(0);
	this.showAddFermd = false;
	this.addFermdForm.reset();
      },
      (err:any) => {
	console.log(err);
	this.errors.push(err.error!.message);
      });
  }

  toggleAddFermd() {
    this.showAddFermd = !this.showAddFermd;
  }

}
