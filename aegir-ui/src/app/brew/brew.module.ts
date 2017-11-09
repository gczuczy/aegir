import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

import { ChartsModule } from 'ng2-charts';

import { BrewComponent } from './brew.component';

@NgModule({
    imports: [
	CommonModule,
	ChartsModule,
	FormsModule
    ],
    declarations: [BrewComponent]
})
export class BrewModule { }
