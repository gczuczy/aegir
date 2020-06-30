import { Component, OnInit } from '@angular/core';
import { HttpErrorResponse } from '@angular/common/http';
import { Validators, FormGroup, FormArray, FormBuilder,
	 ValidatorFn, AbstractControl } from '@angular/forms';
//import { ActivatedRoute, Params, Router } from '@angular/router';
import { switchMap } from 'rxjs/operators';

import { IntValidator } from '../int-validator';
import { ApiService } from '../api.service';

@Component({
  selector: 'app-settings',
  templateUrl: './settings.component.html',
  styleUrls: ['./settings.component.css']
})
export class SettingsComponent implements OnInit {
    public settingsForm: FormGroup;

    // hepower, tempaccuracy, heatoverhead, cooltemp
    public hepower = null;
    public tempaccuracy = null;
    public heatoverhead = null;
    public cooltemp = null;
    public errors: string[];

    constructor(private _fb: FormBuilder,
		private api: ApiService) {
    }

    ngOnInit() {
	this.api.getConfig().subscribe(data => {
	    this.getConfig(data);
	});
    }

    getConfig(data) {
	console.log('settings:getConfig', data);

    }

    save(x) {
    }
}
