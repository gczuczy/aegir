import { Component } from '@angular/core';
import { Router } from '@angular/router';

import { ApiService } from '../api.service';
import { apiProgram } from '../api.types';

@Component({
  selector: 'app-programs',
  templateUrl: './programs.component.html',
  styleUrls: ['./programs.component.css']
})
export class ProgramsComponent {

  programs: apiProgram[] = [];

  constructor(private api: ApiService,
	      private router: Router) {
    this.api.updateAnnounce$.subscribe(
      (u:boolean) => {
	this.getPrograms();
      }
    );
  }

  getPrograms() {
    this.api.getPrograms()
      .subscribe(
	(data:apiProgram[]) => {
	  //console.log('Programs', data);
	  this.programs = data;
	}
      );
  }

  loadProgram(prog: apiProgram) {
    this.router.navigate(['programs', prog.id, 'view']);
  }
}
